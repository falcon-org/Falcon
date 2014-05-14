/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_BUILD_PLAN_H_
#define FALCON_BUILD_PLAN_H_

#include "graph.h"

namespace falcon {

/**
 * BuildPlan is a class that maintains a list of rules we are planning to build.
 * It does not know how to build a rule, but it knows how to schedule them for
 * execution.
 * The builder class can retrieve a rule that is ready to be built with
 * findWork(). When the rule is built, it notifies the build plan with
 * notifyRuleBuilt(). The build plan can then deduce new rules that are ready.
 * This goes on until there are no more rules to build.
 *
 * Usage:
 *
 * BuildPlan plan({ target1, target2, target3 });
 * while (!plan.hasWork()) {
 *   // Find a rule that is ready to be built.
 *   Rule* rule = plan.findWork()
 *   // build the rule.
 *   my_build_function(rule);
 *   // mark the rule as built.
 *   plan.notifyRuleBuilt(rule);
 * }
 *
 */
class BuildPlan {
 public:
  /**
   * Construct a new build plan.
   * @param targets List of targets that we wish to build.
   */
  BuildPlan(NodeSet& targets);

  /**
   * Find a rule that is ready to be built.
   * @return Rule that can be built. It is guaranteed that all its inputs are up
   * to date. Once a rule is returned by findWork, it will be considered
   * "building" and the user is expected to call notifyRuleBuilt() afterwards.
   */
  Rule* findWork();

  /**
   * Determine if we are ready to build more rules.
   * @return true if findWork would return a rule that is ready to be built.
   */
  bool hasWork() const { return !readyRules_.empty(); }

  /**
   * Determine if there are more rules to be built.
   * @return true if all rules have been returned by findWork. If this function
   * returns true, do not call findWork() as it will never return a rule.
   */
  bool done() const { return numStarted_ == rules_.size(); }

  /**
   * Notify that a rule has been built.
   * @param rule Rule that is marked as complete.
   */
  void notifyRuleBuilt(Rule *rule);

 private:

  /**
   * Add a new target to be built.
   * @param target Target to be built.
   */
  void addTarget(Node* target);

  /** All the rules that we need to build in the plan. */
  RuleSet rules_;

  /** Set of rules that are ready to be built, ie all their inputs are up to
   * date. */
  RuleSet readyRules_;

  /**
   * Number of rules that were returned by findWork().
   */
  std::size_t numStarted_;
};

} // namespace falcon

#endif // FALCON_BUILD_PLAN_H_
