/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "graph_parallel_builder.h"
#include "depfile.h"
#include "fs.h"
#include "graph_hash.h"
#include "logging.h"

namespace falcon {

GraphParallelBuilder::GraphParallelBuilder(Graph& graph,
                                           BuildPlan& plan,
                                           CacheManager* cache,
                                           IStreamConsumer* consumer,
                                           WatchmanClient* watchmanClient,
                                           std::string const& workingDirectory,
                                           std::size_t numThreads,
                                           std::mutex& mutex,
                                           onBuildCompletedFn callback)
    : graph_(graph)
    , plan_(plan)
    , cache_(cache)
    , consumer_(consumer)
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
    onRuleFinished(rule);
    return;
  }

  /* Create all the directories for the outputs. */
  auto& outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    /* TODO: we could end the build immediately if this fails. */
    if (!fs::createPath((*it)->getPath())) {
      LOG(ERROR) << "could not create path " << (*it)->getPath();
    }
  }

  /* Create the directory for the depfile. */
  if (rule->hasDepfile() && !fs::createPath(rule->getDepfile())) {
    LOG(ERROR) << "could not create path " << rule->getDepfile();
  }

  if (tryBuildRuleFromCache(rule)) {
    /* We managed to retrieve all the outputs from the cache. */
    onRuleFinished(rule);
    return;
  }

  /* This is not a phony target, and we could not find all the outputs in cache.
   * Build the rule. */
  manager_.addProcess(rule, workingDirectory_);
}

bool GraphParallelBuilder::tryBuildRuleFromCache(Rule *rule) {
  if (!cache_ || rule->isPhony()) {
    return false;
  }

  /* Check we have all the outputs in cache. */
  auto outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!cache_->has((*it)->getHash())) {
      return false;
    }
  }

  /* Retrieve all the outputs. */
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!cache_->read((*it)->getHash(), (*it)->getPath())) {
      return false;
    }
    /* Notify the consumer that we retrieved the target from the cache. */
    consumer_->cacheRetrieveAction((*it)->getPath());
  }

  /* Update the timestamp of the rule. */
  rule->setTimestamp(time(NULL));

  return true;
}

bool GraphParallelBuilder::saveRuleInCache(Rule *rule) {
  if (!cache_ || rule->isPhony()) {
    return false;
  }

  bool error = false;

  auto outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!cache_->update((*it)->getHash(), (*it)->getPath())) {
      LOG(ERROR) << "could not save " << (*it)->getPath() << " in cache";
      error = true;
    }
  }

  if (rule->hasDepfile()) {
    std::string name = rule->getHashDepfile();
    name.append(".deps");
    if (!cache_->update(name, rule->getDepfile())) {
      LOG(ERROR) << "could not save " << rule->getDepfile() << " to "
        << name << std::endl;
      error = true;
    }
  }

  return error;
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

  /* Now that the rule was built, parse its depfile (if any). */
  if (rule->hasDepfile()) {
    auto res = Depfile::loadFromfile(rule->getDepfile(), rule,
        watchmanClient_, graph_, true);
    if (res != Depfile::Res::SUCCESS) {
      return BuildResult::FAILED;
    }
    /* Since the dependencies might have changed, the hash might have
     * changed as well. */
    /* TODO: we should be able to detect that the dependencies did not change
     * and thus not recompute the hashes. */
    hash::recomputeRuleHash(rule, watchmanClient_, graph_, cache_, true, false);
  }

  /* Save the outputs in cache. */
  saveRuleInCache(rule);

  onRuleFinished(rule);
  return BuildResult::SUCCEEDED;
}

void GraphParallelBuilder::onRuleFinished(Rule* rule) {
  /* Mark all the outputs up to date. */
  markOutputsUpToDate(rule);

  /* Mark all the inputs that are source files up to date. */
  auto inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    if ((*it)->getChild() == nullptr) {
      (*it)->setState(State::UP_TO_DATE);
    }
  }

  /* Mark the rule up to date. */
  rule->setState(State::UP_TO_DATE);

  /* Inform the plan that the rule is built. */
  plan_.notifyRuleBuilt(rule);
}

} // namespace falcon
