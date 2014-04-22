/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <iostream>

#include "graph_sequential_builder.h"

namespace falcon {

GraphSequentialBuilder::GraphSequentialBuilder(Graph& graph)
    : graph_(graph), interrupted_(false) { }

GraphSequentialBuilder::~GraphSequentialBuilder() {
  /* Make sure the thread finishes before going out of scope. */
  if (thread_.joinable()) {
    thread_.join();
  }
}

void GraphSequentialBuilder::startBuild(NodeSet& targets,
                                        onBuildCompletedFn cb) {
  assert(cb);
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

  callback_(res);
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

  if (target->getState() == State::UP_TO_DATE) {
    std::cout << target->getPath() << " is up to date" << std::endl;
    return BuildResult::SUCCEEDED;
  }

  Rule *rule = target->getChild();
  if (rule == nullptr) {
    /* This is a source file. */
    target->setState(State::UP_TO_DATE);
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
        return res;
      }
    }
  }

  /* Run the command. */
  if (!rule->isPhony()) {
    assert(!rule->getCommand().empty());
    std::cout << rule->getCommand() << std::endl;

    manager_.addProcess(rule->getCommand());
    PosixSubProcessPtr proc = manager_.waitForNext();

    std::string stdout = proc->flushStdout();
    std::string stderr = proc->flushStderr();
    std::cout << "Command completed." << std::endl;
    std::cout << "STDOUT: [" << stdout << "]" << std::endl;
    std::cout << "STDERR: [" << stderr << "]" << std::endl;

    auto status = proc->status();
    if (status != SubProcessExitStatus::SUCCEEDED) {
      std::cout << "Build failed" << std::endl;
      return BuildResult::FAILED;
    }
  }

  /* Mark the target and the rule up to date. */
  target->setState(State::UP_TO_DATE);
  rule->setState(State::UP_TO_DATE);

  return BuildResult::SUCCEEDED;
}

} // namespace falcon
