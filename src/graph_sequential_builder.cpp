/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <iostream>

#include "graph_sequential_builder.h"
#include "posix_subprocess.h"

namespace falcon {

GraphSequentialBuilder::GraphSequentialBuilder(Graph& graph)
    : graph_(graph) { }

bool GraphSequentialBuilder::build(NodeSet& targets) {
  for (auto it = targets.begin(); it != targets.end(); ++it) {
    if (!buildTarget(*it)) {
      return false;
    }
  }

  return true;
}

bool GraphSequentialBuilder::buildTarget(Node* target) {
  if (target->getState() == State::UP_TO_DATE) {
    return true;
  }

  Rule *rule = target->getChild();
  if (rule == nullptr) {
    /* This is a source file. */
    target->setState(State::UP_TO_DATE);
    return true;
  }

  /* If the target is out of date, the rule should be out of date as well. */
  assert(rule->getState() == State::OUT_OF_DATE);

  /* Build all the necessary inputs. */
  NodeArray& inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    Node* input = *it;
    if (input->getState() == State::OUT_OF_DATE) {
      if (!buildTarget(input)) {
        return false;
      }
    }
  }

  /* Run the command. */
  if (!rule->isPhony()) {
    assert(!rule->getCommand().empty());
    std::cout << rule->getCommand() << std::endl;
    PosixSubProcess process(rule->getCommand());
    auto status = process.start();
    if (status != SubProcessExitStatus::SUCCEEDED) {
      return false;
    }
  }

  /* Mark the target and the rule up to date. */
  target->setState(State::UP_TO_DATE);
  rule->setState(State::UP_TO_DATE);

  return true;
}

} // namespace falcon
