/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>

#include "build_plan.h"
#include "logging.h"

namespace falcon {

BuildPlan::BuildPlan(NodeSet& targets) : numStarted_(0) {
  for (auto it = targets.begin(); it != targets.end(); ++it) {
    addTarget(*it);
  }

  /* If the build plan contains rules to build, we should have at least a rule
   * that is ready otherwise there is no starting point. */
  assert(rules_.empty() || !readyRules_.empty());
}

void BuildPlan::addTarget(Node* target) {
  if (!target->isDirty()) {
    /* This target is already up to date. */
    return;
  }

  Rule* rule = target->getChild();
  if (!rule) {
    /* This is a source file. Ignore it. */
    return;
  }

  if (rules_.find(rule) != rules_.end()) {
    /* This rule is already in the plan. */
    return;
  }

  /* If the target is dirty, the rule should be dirty as well. */
  assert(rule->isDirty());
  rules_.insert(rule);

  if (rule->ready()) {
    /* All the inputs are already built. */
    assert(readyRules_.find(rule) == readyRules_.end());
    readyRules_.insert(rule);
  } else {
    /* Traverse the graph to add any other rule that will build the required
     * inputs. */
    auto& inputs = rule->getInputs();
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
      addTarget(*it);
    }
  }
}

Rule* BuildPlan::findWork() {
  if (readyRules_.empty()) {
    /* We cannot build any rule. */
    return nullptr;
  }
  auto itRule = readyRules_.begin();
  Rule* rule = *itRule;
  readyRules_.erase(itRule);
  assert(rule);
  numStarted_++;

  return rule;
}

void BuildPlan::notifyRuleBuilt(Rule *rule) {
  auto itRule = rules_.find(rule);
  assert(readyRules_.find(rule) == readyRules_.end());
  assert(itRule != rules_.end());

  /* Traverse the outputs to find any rule that became ready. */
  auto& outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    auto& parentRules = (*it)->getParents();
    for (auto it2 = parentRules.begin(); it2 != parentRules.end(); ++it2) {
      Rule *parentRule = *it2;
      /* If the rule is ready and in the plan, add it to readyRules_. */
      if (parentRule->ready() && rules_.find(parentRule) != rules_.end()) {
        readyRules_.insert(parentRule);
      }
    }
  }
}

} // namespace falcon
