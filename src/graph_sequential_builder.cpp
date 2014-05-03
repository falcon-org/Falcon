/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <iostream>
#include <stack>

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

  depth_++;

  LOG(debug) << "(" << depth_ << ") building " << target->getPath();

  if (target->getState() == State::UP_TO_DATE) {
    LOG(trace) << "(" << depth_-- << ")" << "target is up to date";
    return BuildResult::SUCCEEDED;
  }

  Rule *rule = target->getChild();
  if (rule == nullptr) {
    /* This is a source file. */
    target->setState(State::UP_TO_DATE);
    depth_--;
    return BuildResult::SUCCEEDED;
  }

  /* Build all the necessary inputs. */
  NodeArray& inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    Node* input = *it;
    if (input->getState() == State::OUT_OF_DATE) {
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

    auto status = proc->status();
    if (status != SubProcessExitStatus::SUCCEEDED) {
      return status == SubProcessExitStatus::INTERRUPTED ?
        BuildResult::INTERRUPTED : BuildResult::FAILED;
    }

    NodeArray& outputs = rule->getOutputs();
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      (*it)->setState(State::UP_TO_DATE);
    }

    /* TODO: Update the implicit dependencies by parsing the depfile, if any. */
    if (rule->hasDepfile()) {
      auto res = Depfile::loadFromfile(rule->getDepfile(), rule,
                                       watchmanClient_, graph_);
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
