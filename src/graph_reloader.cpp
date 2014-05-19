/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_reloader.h"
#include "cache_manager.h"
#include "graph_hash.h"
#include "graph_dependency_scan.h"
#include "depfile.h"
#include "logging.h"

#include <cassert>
#include <algorithm>

namespace falcon {

GraphReloader::GraphReloader(Graph& original, Graph& newGraph,
                             WatchmanClient& watchman)
  : original_(original)
  , new_(newGraph)
  , watchman_(watchman)
  , originalNodes_()
  , nodesSeen_()
{
}

void GraphReloader::updateGraph() {
  updateRoots();
  cleanSlateSubgraph();
}

std::pair<Node*, bool> GraphReloader::getNode(NodeArray& source, Node* newNode)
{
  auto nodeIt = std::find_if(source.begin(), source.end(),
      [newNode](Node* n) { return newNode->getPath() == n->getPath(); }
    );

  if (nodeIt != source.end()) {
    Node* node = *nodeIt;
    source.erase(nodeIt);
    originalNodes_.erase(node->getPath());
    return std::make_pair(node, true);
  }

  return std::make_pair(getNodeFromAll(newNode), false);
}

/* Helper to find a Node from:
 *  -- originalNodes_ (the node comes from the original graph and hasn't been
 *                     visited yet)
 *  or original_ (the node has been visited or created and already exist in the
 *                updated graph)
 *  or create it */
Node* GraphReloader::getNodeFromAll(Node* newNode) {
  auto it = original_.nodes_.find(newNode->getPath());

  /* This is a new node. */
  if (it == original_.nodes_.end()) {
    Node* node = new Node(newNode->getPath(), true);
    original_.nodes_[node->getPath()] = node;
    watchman_.watchNode(*node);
    return node;
  }

  /* This node already exists. */
  Node* node = it->second;
  originalNodes_.erase(node->getPath());
  return node;
}

void GraphReloader::updateRoots() {
  originalNodes_ = original_.nodes_;
  original_.roots_.clear();
  original_.sources_.clear();

  for (auto it = new_.getRoots().cbegin();
       it != new_.getRoots().cend();
       ++it) {
    Node* root = getNodeFromAll(*it);

    bool ret = updateSubGraph(root, *it);
    if (ret) {
      hash::updateNodeHash(*root, true, true);
    }

    original_.roots_.insert(root);
  }
}

void GraphReloader::cleanSlateSubgraph() {
  /* Delete no longer needed nodes */
  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    assert(it->second->parentRules_.empty());

    /* request watchman stop to wath it */
    watchman_.unwatchNode(*(it->second));

    if (!it->second->isSource()) {
      deleteChildRule(it->second);
      /* TODO: At this point, the output can be deleted? */
    }

    /* TODO: do we really need to do that since we cleared original_? */
    original_.nodes_.erase(it->first);
    delete it->second;
  }
}

bool GraphReloader::updateSubGraph(Node* node, Node const* newNode) {
  bool r = false;

  if (nodesSeen_.find(node) != nodesSeen_.end()) {
    return r;
  }
  nodesSeen_.insert(node);

  if (node->isSource() && newNode->isSource()) {
    DLOG(INFO) << "  node '" << node->getPath() << "' stays a source file";
    /* Nothing to change here, the node still a source file
     * just insert it in the Sources Set */
    original_.sources_.insert(node);
  } else if (!node->isSource() && newNode->isSource()) {
    DLOG(INFO) << "  node '" << node->getPath() << "' is now a source file";
    /* The node is now a source */
    deleteChildRule(node);
    statNode(node);
    original_.sources_.insert(node);
    r = true;
  } else { /* !newNode->isSource() */
    if (node->isSource()) {
      DLOG(INFO) << "  node '" << node->getPath() << "' is now an output";
      /* The Node used to be a Source file. But is now an output of a rule.
       * Create it: */
      createNewRule(newNode->getChild(), node);
    }
    /* Update the sub rule */
    r = updateSubGraph(node->getChild(), newNode->getChild());
    if (r) {
      hash::updateRuleHash(*node->getChild(), true, true);
    }
  }

  if (r) {
    node->setState(State::OUT_OF_DATE);
  }

  original_.nodes_[node->getPath()] = node;
  return r;
}

bool GraphReloader::updateRuleInputs(Rule* rule, NodeArray& inputs,
                                     Rule const* newRule) {
  bool r = false;

  /* Build inputs */
  for (auto it = newRule->inputs_.begin(); it != newRule->inputs_.end(); ++it) {
    /* look if the node comes from the original rule */
    auto ret = getNode(inputs, *it);
    Node* node = ret.first;
    if (!ret.second) {
      /* The node wasn't in the original rule's input:
       * So this is a new node (at least from the rule point of view)
       *
       * The rule has been changed: we need to update the hash */
      r = true;

      /* Since the node wasn't yet in the rule's inputs, we need to add the
       * rule in its parent rules */
      node->addParentRule(rule);
    }

    /* Update the input subgraph */
    r |= updateSubGraph(node, *it);
    if (r) {
      /* TODO: here we might be re-computing the hash of a source file several
       * times if this source file is new for several rules... */
      hash::updateNodeHash(*node, true, true);
    }

    rule->inputs_.push_back(node);
    if (node->isSource() || node->getState() == State::UP_TO_DATE) {
      rule->markInputReady();
    }
  }
  /* Remove no longer needed inputs */
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    (*it)->removeParentRule(rule);
    clearSubGraph(*it);
    /* At this point, we don't need to really delete the node:
     * the node may be needed in future and if not it will be deleted at the
     * end */
  }
  return r;
}

bool GraphReloader::updateRuleOutputs(Rule* rule, Rule const* newRule) {
  bool r = false;

  NodeArray outputs = std::move(rule->outputs_); /* Save the outputs */

  /* build the rule's outputs */
  for (auto it = newRule->outputs_.begin(); it != newRule->outputs_.end(); ++it) {
    /* get it from the original output */
    auto ret = getNode(outputs, *it);
    Node* node = ret.first;
    if (!ret.second) {
      /* This is a new output (at least from the rule point of view):
       * So the rule has been changed: we will need to update the hash: */
      r = true;

      /* Get it from the already existing Node or create it */
      node = getNodeFromAll(*it);

      /* The node is an output of the current rule, so set the Child Rule */
      node->childRule_ = rule;
    }

    rule->outputs_.push_back(node);
  }
  /* Delete the no longer needed ouputs */
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    deleteChildRule(*it);
    /* At this point, we don't need to really delete the node:
     * the node may be needed in future and if not it will be deleted at the
     * end */
  }
  return r;
}

bool GraphReloader::updateRuleDepfiles(Rule* rule,
                                       NodeArray& implicitDepsBefore,
                                       Rule const* newRule) {
  if (rule->getDepfile() != newRule->getDepfile()) {
    /* Delete the old implicitDeps */
    for (auto it = implicitDepsBefore.begin();
         it != implicitDepsBefore.end(); ++it) {
      (*it)->removeParentRule(rule);
    }
    /* This is a new depfile:
     * so: reset the depfile, and clear the implicitDeps */
    rule->setDepfile(newRule->getDepfile());
    Depfile::loadFromfile(rule->getDepfile(), rule,
                          &watchman_, original_, false);
    return true;
  }

  /* The depfile remains the same: re-insert the implicitDeps. */
  for (auto it = implicitDepsBefore.begin(); it != implicitDepsBefore.end(); ++it) {
    rule->inputs_.push_back(*it);
    originalNodes_.erase((*it)->getPath());
    original_.nodes_[(*it)->getPath()] = *it;
    if ((*it)->isSource()) {
      original_.sources_.insert(*it);
    }
    if ((*it)->isSource() || (*it)->getState() == State::UP_TO_DATE) {
      rule->markInputReady();
    }
  }
  rule->numImplicitDeps_ = implicitDepsBefore.size();

  return false;
}

/* Even if there is no modification, we un-build and then re-build the rule */
bool GraphReloader::updateSubGraph(Rule* rule, Rule const* newRule) {
  NodeArray inputs(rule->inputs_.begin(),
                   rule->inputs_.end() - rule->numImplicitDeps_);
  NodeArray implicitDepsBefore(rule->inputs_.end() - rule->numImplicitDeps_,
                               rule->inputs_.end());

  rule->inputs_.clear();
  rule->numInputsReady_ = 0;
  rule->numImplicitDeps_ = 0;

  bool r = updateRuleInputs(rule, inputs, newRule);
  r |= updateRuleOutputs(rule, newRule);

  if (rule->getCommand() != newRule->getCommand()) {
    rule->setCommand(newRule->getCommand());
    r = true;
  }

  /* Rebuild depfile */
  r |= updateRuleDepfiles(rule, implicitDepsBefore, newRule);

  /* If the rule has been changed, mark this rule dirty */
  if (r) {
    rule->setState(State::OUT_OF_DATE);
  }

  return r;
}

void GraphReloader::deleteChildRule(Node* node) {
  assert(!node->isSource());

  Rule* rule = node->getChild();
  node->childRule_ = nullptr;

  { /* Remove this node from the output of the child rule */
    auto nodeIt = std::find(rule->outputs_.begin(), rule->outputs_.end(), node);
    assert(nodeIt != rule->outputs_.end());
    rule->outputs_.erase(nodeIt);
  }

  if (rule->outputs_.empty()) {
    /* In the case there is no longer ouputs for this Rule, we can delete it
     * -1- remove it from the parentRules set in its input nodes */
    for (auto it = rule->inputs_.begin(); it != rule->inputs_.end(); ++it) {
      (*it)->removeParentRule(rule);
    }
    { /* -2- delete it from the graph rule Array */
      auto ruleIt = std::find(original_.rules_.begin(),
                              original_.rules_.end(), rule);
      assert(ruleIt != original_.rules_.end());
      original_.rules_.erase(ruleIt);
    }
    delete rule;
  }
}

Rule* GraphReloader::createNewRule(Rule const* newRule, Node* output) {
  NodeArray inputs;
  NodeArray outputs;

  Rule* rule = new Rule(inputs, outputs);

  rule->outputs_.push_back(output);
  output->setChild(rule);

  original_.rules_.push_back(rule);

  return rule;
}

void GraphReloader::clearSubGraph(Node* n) {
  if (n->isSource()) {
    return;
  }

  Rule* r = n->getChild();
  for (auto it = r->getInputs().begin(); it != r->getInputs().end(); ++it) {
    clearSubGraph(*it);
  }

  deleteChildRule(n);
}

}
