/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <cassert>
#include <sstream>

#include "graph_consistency_checker.h"

#include "logging.h"

namespace falcon {

/* ************************************************************************* */
/* Useful macros for performing checks.                              */
/* ************************************************************************* */

/* Useful macros for checking a condition, logging an error if it is unmet.
 * Must only be called from within a method of GraphConsistencyChecker. */

#define FCHECK(__cond__) \
  Checker(__cond__, this) << "Graph is inconsistent: " << #__cond__ << ". "

#define FCHECK_EQ(__got__, __expect__)                            \
  FCHECK((__expect__) == (__got__)) <<                            \
  "Expected " << #__expect__ << " which is " << (__expect__) <<  \
  " but got " << #__got__ << " which is " << (__got__) << ". "

/* ************************************************************************* */
/* Graph Consistency checker                                                 */
/* ************************************************************************* */

GraphConsistencyChecker::GraphConsistencyChecker(Graph* graph)
  : graph_(graph)
  , nbRootsSeen_(0)
  , nbSourcesSeen_(0)
  , isConsistent_(true) { }

void GraphConsistencyChecker::check() {
  auto roots = graph_->getRoots();

  for (auto it = roots.begin(); it != roots.end(); it++) {
    checkNode(*it);
  }

  FCHECK_EQ(nodesSeen_.size(), graph_->getNodes().size())
    << "Invalid number of nodes.";
  FCHECK_EQ(nbRootsSeen_, graph_->getRoots().size())
    << "Invalid number of roots.";
  FCHECK_EQ(nbSourcesSeen_, graph_->getSources().size())
    << "Invalid number of sources.";
  FCHECK_EQ(rulesSeen_.size(), graph_->getRules().size())
    << "Invalid number of rules.";
}

void GraphConsistencyChecker::checkNode(Node* node) {
  if (nodesSeen_.find(node) != nodesSeen_.end()) {
    return;
  }
  nodesSeen_.insert(node);

  /* Check that the node is in the map. */
  FCHECK(graph_->getNodes().find(node->getPath()) != graph_->getNodes().end())
    << node->getPath() << " not found in the map.";

  if (node->getParents().empty()) {
    /* This node has no parent, this must be a root. */
    FCHECK(graph_->getRoots().find(node) != graph_->getRoots().end())
      << node->getPath() << " has no parent but is not in list of root nodes.";
    nbRootsSeen_++;
  }

  if (node->getChild() == nullptr) {
    /* This node has no child rule, this must be a source file. */
    FCHECK(graph_->getSources().find(node) != graph_->getSources().end())
      << node->getPath() << " has no child rule, but is not in list of source"
      << " files.";
    nbSourcesSeen_++;
  } else {
    /* If a node is out of date, it's child rule must be as well. */
    if (node->isDirty()) {
      FCHECK(node->getChild()->isDirty())
        << node->getPath() << " is out of date, but it's child rule is not.";
    }
    checkRule(node->getChild());
  }
}

void GraphConsistencyChecker::checkRule(Rule* rule) {
  if (rulesSeen_.find(rule) != rulesSeen_.end()) {
    return;
  }
  rulesSeen_.insert(rule);

  auto outputs = rule->getOutputs();
  auto inputs = rule->getInputs();

  /* A rule must have at least one output. */
  FCHECK(!outputs.empty()) << "Rule " << rule << " has no output.";

  FCHECK_EQ(rule->isPhony(), rule->getCommand().empty())
    << "A phony rule should have an empty command.";

  if (rule->isPhony()) {
    /* A phony rule should have only one output. */
    FCHECK_EQ(outputs.size(), 1) << "Rule " << rule << " is phony but has more "
      << "than one output.";
  }

  /* If a rule is out of date, its outputs must be as well. */
  if (rule->isDirty()) {
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
      FCHECK((*it)->isDirty()) << "Rule " << rule << " is dirty but it's output "
        << (*it)->getPath() << " is not.";
    }
  }

  /* Traverse inputs. */
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    if ((*it)->isDirty()) {
      FCHECK(rule->isDirty()) << "Input " << (*it)->getPath() << " of rule " <<
        rule << " is dirty but the rule is not.";
    }
    checkNode(*it);
  }
}

} // namespace falcon
