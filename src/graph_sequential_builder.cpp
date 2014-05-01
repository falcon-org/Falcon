/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <iostream>

#include "logging.h"
#include "graph_sequential_builder.h"

namespace falcon {

GraphSequentialBuilder::GraphSequentialBuilder(Graph& graph,
    std::string const& workingDirectory, IStreamConsumer* consumer)
    : manager_(consumer)
    , graph_(graph)
    , workingDirectory_(workingDirectory)
    , interrupted_(false)
    , depth_(0) { }

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
  auto res = BuildResult::SUCCEEDED;
  for (auto it = targets.begin(); it != targets.end(); ++it) {
    res = buildTarget(*it);
    if (res != BuildResult::SUCCEEDED) {
      break;
    }
  }

  if (callback_) {
    callback_(res);
  }
}

void GraphSequentialBuilder::interrupt() {
  interrupted_ = true;
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

  /* If the target is out of date, the rule should be out of date as well. */
  assert(rule->getState() == State::OUT_OF_DATE);

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

  /* Run the command. */
  if (!rule->isPhony()) {
    assert(!rule->getCommand().empty());
    std::cout << rule->getCommand() << std::endl;

    manager_.addProcess(rule->getCommand(), workingDirectory_);
    PosixSubProcessPtr proc = manager_.waitForNext();

    auto status = proc->status();
    if (status != SubProcessExitStatus::SUCCEEDED) {
      LOG(error) << "(" << depth_-- << ") Build failed" << std::endl;
      return BuildResult::FAILED;
    }

    NodeArray& outputs = rule->getOutputs();
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      (*it)->setState(State::UP_TO_DATE);
    }
  }

  /* Mark the target and the rule up to date. */
  target->setState(State::UP_TO_DATE);
  rule->setState(State::UP_TO_DATE);

  depth_--;
  return BuildResult::SUCCEEDED;
}

} // namespace falcon
