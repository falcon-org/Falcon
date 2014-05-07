/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_parallel_builder.h"
#include "depfile.h"

#include "logging.h"

namespace falcon {

GraphParallelBuilder::GraphParallelBuilder(Graph& graph,
                                           BuildPlan& plan,
                                           IStreamConsumer* consumer,
                                           WatchmanClient* watchmanClient,
                                           std::string const& workingDirectory,
                                           std::size_t numThreads,
                                           std::mutex& mutex,
                                           onBuildCompletedFn callback)
    : graph_(graph)
    , plan_(plan)
    , manager_(consumer)
    , watchmanClient_(watchmanClient)
    , workingDirectory_(workingDirectory)
    , numThreads_(numThreads)
    , result_(BuildResult::SUCCEEDED)
    , lock_(mutex, std::defer_lock)
    , interrupted_(false)
    , callback_(callback) {}

GraphParallelBuilder::~GraphParallelBuilder() {
  /* Make sure the thread finishes before going out of scope. */
  if (thread_.joinable()) {
    thread_.join();
  }
}

void GraphParallelBuilder::startBuild() {
  thread_ = std::thread(std::bind(&GraphParallelBuilder::buildThread, this));
}

void GraphParallelBuilder::interrupt() {
  interrupted_ = true;
  manager_.interrupt();
}

void GraphParallelBuilder::wait() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void GraphParallelBuilder::buildThread() {
  /* TODO: improve locking mechanism so that the whole graph is not locked while
   * building. */
  lock_.lock();

  /* Main build loop. */
  while (result_ == BuildResult::SUCCEEDED
      && !plan_.done() && !interrupted_) {

    /* Try to spawn as many ready commands as possible. */
    while (plan_.hasWork() && manager_.nbRunning() < numThreads_) {
      Rule *rule = plan_.findWork();
      assert(rule);
      buildRule(rule);
    }

    /* We cannot run any more work. Wait for a command to complete. */
    if (manager_.nbRunning() > 0) {
      result_ = waitForNext();
    }
  }

  /* Flush the process manager. */
  while (manager_.nbRunning() > 0) {
    BuildResult tmp = waitForNext();
    if (result_ == BuildResult::SUCCEEDED) {
      result_ = tmp;
    }
  }

  lock_.unlock();
  if (callback_) {
    callback_(result_);
  }
}

void GraphParallelBuilder::buildRule(Rule* rule) {
  if (rule->isPhony()) {
    /* A phony target, nothing to do. */
    markOutputsUpToDate(rule);
    rule->setState(State::UP_TO_DATE);
    plan_.notifyRuleBuilt(rule);
  } else {
    manager_.addProcess(rule, workingDirectory_);
  }
}

void GraphParallelBuilder::markOutputsUpToDate(Rule *rule) {
  auto outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    (*it)->setState(State::UP_TO_DATE);
    /* Notify the parents of this output that one of their inputs is ready. */
    auto parentRules = (*it)->getParents();
    for (auto it2 = parentRules.begin(); it2 != parentRules.end(); ++it2) {
      (*it2)->markInputReady();
    }
  }
}

BuildResult GraphParallelBuilder::waitForNext() {
  auto res = manager_.waitForNext();
  Rule* rule = res.first;
  SubProcessExitStatus status = res.second;

  /* Update the timestamp of the rule. */
  rule->setTimestamp(std::time(NULL));

  if (status != SubProcessExitStatus::SUCCEEDED) {
    return status == SubProcessExitStatus::INTERRUPTED ?
        BuildResult::INTERRUPTED : BuildResult::FAILED;
  }

  /* Mark all the outputs up to date. */
  markOutputsUpToDate(rule);

  /* Mark all the inputs that are source files up to date.
   * Contrary to the inputs that are generated, this is done only after the
   * command succeeds, because the source files must remain dirty if the
   * command fails. */
  auto inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    if ((*it)->getChild() == nullptr) {
      (*it)->setState(State::UP_TO_DATE);
    }
  }

  /* Now that the rule was built, parse its depfile (if any). */
  if (rule->hasDepfile()) {
    auto res = Depfile::loadFromfile(rule->getDepfile(), rule,
        watchmanClient_, graph_, true);
    if (res != Depfile::Res::SUCCESS) {
      return BuildResult::FAILED;
    }
  }

  /* Mark the rule up to date. */
  rule->setState(State::UP_TO_DATE);

  /* Inform the plan that the rule is built. */
  plan_.notifyRuleBuilt(rule);

  return BuildResult::SUCCEEDED;
}

} // namespace falcon
