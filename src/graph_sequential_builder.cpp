/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <iostream>
#include <stack>
#include <sys/stat.h>

#include "graph_sequential_builder.h"
#include "watchman.h"
#include "depfile.h"
#include "logging.h"

namespace falcon {

std::string toString(BuildResult v) {
  switch (v) {
    case BuildResult::UNKNOWN: return "UNKNOWN";
    case BuildResult::SUCCEEDED: return "SUCCEEDED";
    case BuildResult::INTERRUPTED: return "INTERRUPTED";
    case BuildResult::FAILED: return "FAILED";
    default: THROW_ERROR(EINVAL, "Unrecognized BuildResult");
  }
}

GraphSequentialBuilder::GraphSequentialBuilder(Graph& graph, std::mutex& mutex,
    WatchmanClient* watchmanClient, std::string const& workingDirectory,
    IStreamConsumer* consumer)
    : manager_(consumer)
    , watchmanClient_(watchmanClient)
    , graph_(graph)
    , lock_(mutex, std::defer_lock)
    , workingDirectory_(workingDirectory)
    , interrupted_(false)
    , depth_(0)
    , res_(BuildResult::UNKNOWN) { }

GraphSequentialBuilder::~GraphSequentialBuilder() {
  /* Make sure the thread finishes before going out of scope. */
  if (thread_.joinable()) {
    thread_.join();
  }
}

void GraphSequentialBuilder::startBuild(NodeSet& targets,
                                        onBuildCompletedFn cb) {
  interrupted_ = false;
  callback_ = cb;

  thread_ = std::thread(std::bind(&GraphSequentialBuilder::buildThread,
                        this, targets));
}

void GraphSequentialBuilder::buildThread(NodeSet& targets) {
  res_ = BuildResult::SUCCEEDED;

  /* TODO: improve locking mechanism so that the whole graph is not locked while
   * building. */
  lock_.lock();

  for (auto it = targets.begin(); it != targets.end(); ++it) {
    res_ = buildTarget(*it);
    if (res_ != BuildResult::SUCCEEDED) {
      break;
    }
  }

  lock_.unlock();

  if (callback_) {
    callback_(res_);
  }
}

void GraphSequentialBuilder::interrupt() {
  interrupted_ = true;
  manager_.interrupt();
}

void GraphSequentialBuilder::wait() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

BuildResult GraphSequentialBuilder::buildTarget(Node* target) {
  if (interrupted_) {
    return BuildResult::INTERRUPTED;
  }

  /* This should not be called on a source file. */
  assert(target->getChild() != nullptr);

  depth_++;

  DLOG(INFO) << "(" << depth_ << ") building " << target->getPath();

  if (target->getState() == State::UP_TO_DATE) {
    DLOG(INFO) << "(" << depth_-- << ")" << "target is up to date";
    return BuildResult::SUCCEEDED;
  }

  /* Build all the necessary inputs that are not source files. */
  Rule *rule = target->getChild();
  NodeArray& inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    Node* input = *it;
    if (input->getState() == State::OUT_OF_DATE
          && input->getChild() != nullptr) {
      auto res = buildTarget(input);
      if (res != BuildResult::SUCCEEDED) {
        depth_--;
        return res;
      }
    }
  }

  depth_--;

  /* Run the command. */
  if (!rule->isPhony()) {
    assert(!rule->getCommand().empty());
    manager_.addProcess(rule->getCommand(), workingDirectory_);
    PosixSubProcessPtr proc = manager_.waitForNext();

    /* Update the timestamp of the rule. */
    rule->setTimestamp(time(NULL));

    auto status = proc->status();
    if (status != SubProcessExitStatus::SUCCEEDED) {
      return status == SubProcessExitStatus::INTERRUPTED ?
        BuildResult::INTERRUPTED : BuildResult::FAILED;
    }

    /* Mark all the outputs up to date. */
    NodeArray& outputs = rule->getOutputs();
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      (*it)->setState(State::UP_TO_DATE);
    }

    /* Mark all the inputs that are source files up to date.
     * Contrary to the inputs that are generated, this is done only after the
     * command succeeds, because the source files must remain dirty if the
     * command fails. */
    for (auto it = inputs.begin(); it != inputs.end(); it++) {
      if ((*it)->getChild() == nullptr) {
        (*it)->setState(State::UP_TO_DATE);
      }
    }

    if (rule->hasDepfile()) {
      auto res = Depfile::loadFromfile(rule->getDepfile(), rule,
                                       watchmanClient_, graph_, true);
      if (res != Depfile::Res::SUCCESS) {
        return BuildResult::FAILED;
      }
    }
  }

  /* Mark the target and the rule up to date. */
  target->setState(State::UP_TO_DATE);
  rule->setState(State::UP_TO_DATE);

  return BuildResult::SUCCEEDED;
}

} // namespace falcon
