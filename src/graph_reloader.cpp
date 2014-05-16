/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_reloader.h"
#include "cache_manager.h"
#include "graph_hash.h"
#include "graph_dependency_scan.h"
#include "depfile.h"
#include "depfile.h"
#include "logging.h"

#include <cassert>
#include <algorithm>

namespace falcon {

GraphReloader::GraphReloader(Graph* original, Graph* newGraph,
                             WatchmanClient& watchman,
                             CacheManager* cache)
  : original_(original)
  , new_(newGraph)
  , watchman_(watchman)
  , cache_(cache)
  , originalNodes_()
  , nodesSeen_()
{
  updateRoots();
}

GraphReloader::~GraphReloader() {
  delete new_;
}

Graph* GraphReloader::getUpdatedGraph() {
  return original_;
}

void GraphReloader::updateRoots() {
  /* Rebuild the Graph from the roots: */
  originalNodes_ = original_->nodes_;
  original_->nodes_.clear();
  original_->roots_.clear();
  original_->sources_.clear();

  for (auto it = new_->getRoots().cbegin();
       it != new_->getRoots().cend();
       ++it) {
    auto rootIt = originalNodes_.find((*it)->getPath());
    Node* root = nullptr;

    if (rootIt == originalNodes_.end()) {
      /* TODO: even if the node has been already visited, we don't know yet if
       * it is a root node. So, we may need to look on the original_->nodes_ */
      /* This is a new root (a new Node): create it. */
      root = createNewNode(*it);
    } else {
      root = rootIt->second;
      /* Remove it from the originalNodes */
      originalNodes_.erase(rootIt);
    }
    res ret = updateSubGraph(root, *it);
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateNodeHash(*root, ret.updateHash, ret.updateDepsHash);
      root->setState(State::OUT_OF_DATE);
    }

    original_->roots_.insert(root);
  }

  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    /* request watchman stop to wath it */
    DLOG(INFO) << "delete node: " << it->first << " parents("
               << it->second->parentRules_.size() << ")";
    assert(it->second->parentRules_.empty());
    watchman_.unwatchNode(*(it->second));

    if (!it->second->isSource()) {
      deleteChildRule(it->second);
    }
    original_->nodes_.erase(it->first);
  }
  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    delete it->second;
  }
}

GraphReloader::res
GraphReloader::updateSubGraph(Node* node, Node const* newNode) {
  res r = { .updateHash = false, .updateDepsHash = false };

  if (nodesSeen_.find(node) != nodesSeen_.end()) {
    return r;
  }
  nodesSeen_.insert(node);

  DLOG(INFO) << "update Node(" << node->getPath() << ")";

  if (node->isSource() && newNode->isSource()) {
    /* Nothing to change here, the node still a source file
     * just insert it in the Graph sources set */
    original_->sources_.insert(node);
  } else if (node->isSource() && !newNode->isSource()) {
    /* The Node used to be a Source file. But is now an output of a rule.
     * Create it: */
    createNewRule(newNode->getChild());
    r.updateHash = true;
  } else if (!node->isSource() && newNode->isSource()) {
    /* The node is now a source */
    deleteChildRule(node);
    statNode(node);
    original_->sources_.insert(node);
    r.updateHash = true;
  } else {
    /* Update the sub rule */
    res ret = updateSubGraph(node->getChild(), newNode->getChild());
    hash::updateRuleHash(*node->getChild(), ret.updateHash, ret.updateDepsHash);
    r.updateHash = ret.updateHash;
    r.updateDepsHash = ret.updateDepsHash;
  }

  if (r.updateHash || r.updateDepsHash) {
    node->setState(State::OUT_OF_DATE);
  }

  original_->nodes_[node->getPath()] = node;
  return r;
}

GraphReloader::res
GraphReloader::updateSubGraph(Rule* rule, Rule const* newRule) {
  res r = { .updateHash = false, .updateDepsHash = false };

  NodeArray inputs(rule->inputs_.begin(),
                   rule->inputs_.end() - rule->numImplicitDeps_);
  NodeArray implicitDepsBefore(rule->inputs_.end() - rule->numImplicitDeps_,
                               rule->inputs_.end());

  rule->inputs_.clear();
  rule->numInputsReady_ = 0;

  for (auto it = newRule->inputs_.begin(); it != newRule->inputs_.end(); ++it) {
    Node* node = nullptr;

    { /* retrieve or create the corresponding node
       * -1- try to find it in the current input */
      auto nodeIt = std::find_if(inputs.begin(), inputs.end(),
          [it](Node* n) { return (*it)->getPath() == n->getPath(); }
        );

      if (nodeIt != inputs.end()) {
        /* If the node was already in the input */
        node = *nodeIt;
        inputs.erase(nodeIt);
        originalNodes_.erase(node->getPath());
      } else {
        /* The rule has changed, we need to update the hash */
        r.updateHash = true;

        /* -2- we are going to add a new Input, try to find it in the existing
         * Nodes or creat it if needed */
        auto nodeItPair = originalNodes_.find((*it)->getPath());
        if (nodeItPair == originalNodes_.end()) {
          nodeItPair = original_->nodes_.find((*it)->getPath());
          if (nodeItPair == originalNodes_.end()) {
            node = createNewNode(*it);
          } else {
            node = nodeItPair->second;
          }
        } else {
          node = nodeItPair->second;
          originalNodes_.erase(nodeItPair);
        }
        /* Since the node wasn't yet in the rule inputs, we need to add the
         * rule in its parent rules */
        node->addParentRule(rule);
      }
    }

    /* Update the input subgraph */
    assert(node->isExplicitDependency());
    res ret = updateSubGraph(node, *it);
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateNodeHash(*node, ret.updateHash, ret.updateDepsHash);
    }
    r.updateHash |= ret.updateHash;
    r.updateDepsHash |= ret.updateDepsHash;

    DLOG(INFO) << "add input node: " << node->getPath();;
    rule->inputs_.push_back(node);
    if (node->isSource() || node->getState() == State::UP_TO_DATE) {
      rule->markInputReady();
    }
  }
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    deleteParentRule(*it, rule);
    clearSubGraph(*it);
    DLOG(INFO) << "remove input: " << (*it)->getPath();
  }

  if (rule->getDepfile() != newRule->getDepfile()) {
    r.updateDepsHash = true;
    rule->setDepfile(newRule->getDepfile());
    rule->numImplicitDeps_ = 0;
    Depfile::loadFromfile(rule->getDepfile(), rule,
                          &watchman_, *original_, true);
    for (auto it = implicitDepsBefore.begin();
         it != implicitDepsBefore.end(); ++it) {
      deleteParentRule(*it, rule);
    }
  } else {
    /* re-insert the deps */
    for (auto it = implicitDepsBefore.begin(); it != implicitDepsBefore.end(); ++it) {
      DLOG(INFO) << "Re-use dep file: " << (*it)->getPath();
      assert(!(*it)->isExplicitDependency());
      rule->inputs_.push_back(*it);
      originalNodes_.erase((*it)->getPath());
      original_->nodes_[(*it)->getPath()] = *it;
      if ((*it)->isSource() || (*it)->getState() == State::UP_TO_DATE) {
        rule->markInputReady();
      }
    }
  }

  NodeArray outputs(rule->outputs_);
  rule->outputs_.clear();

  for (auto it = newRule->outputs_.begin(); it != newRule->outputs_.end(); ++it) {
    Node* node = nullptr;

    { /* retrieve or create the corresponding node
       * -1- try to find it in the current input */
      auto nodeIt = std::find_if(outputs.begin(), outputs.end(),
          [it](Node* n) { return (*it)->getPath() == n->getPath(); }
        );

      if (nodeIt != outputs.end()) {
        /* If the node was already in the input */
        node = *nodeIt;
        outputs.erase(nodeIt);
        originalNodes_.erase(node->getPath());
      } else {
        /* The rule has changed, we need to update the hash */
        r.updateHash = true;

        /* -2- we are going to add a new Input, try to find it in the existing
         * Nodes or creat it if needed */
        auto nodeItPair = originalNodes_.find((*it)->getPath());
        if (nodeItPair == originalNodes_.end()) {
          nodeItPair = original_->nodes_.find((*it)->getPath());
          if (nodeItPair == originalNodes_.end()) {
            node = createNewNode(*it);
          } else {
            node = nodeItPair->second;
          }
        } else {
          node = nodeItPair->second;
          originalNodes_.erase(nodeItPair);
        }
      }
    }

    DLOG(INFO) << "add output node: " << node->getPath();;
    rule->outputs_.push_back(node);
  }
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    deleteChildRule(*it);
    DLOG(INFO) << "delete child rule of " << (*it)->getPath();
  }

  if (rule->getCommand() != newRule->getCommand()) {
    rule->setCommand(newRule->getCommand());
    r.updateHash = true;
    r.updateDepsHash = true;
  }

  if (r.updateHash || r.updateDepsHash) {
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
      DLOG(INFO) << "delete parent rule of " << (*it)->getPath();
      deleteParentRule(*it, rule);
    }
    { /* -2- delete it from the graph rule Array */
      auto ruleIt = std::find(original_->rules_.begin(),
                              original_->rules_.end(), rule);
      assert(ruleIt != original_->rules_.end());
      original_->rules_.erase(ruleIt);
    }
    delete rule;
  }
}

/* Delete the reference of the given rule in the parent rules of the given
 * Node. */
void GraphReloader::deleteParentRule(Node* node, Rule const* rule) {
  auto ruleIt = std::find(node->parentRules_.begin(), node->parentRules_.end(),
                          rule);
  if (ruleIt != node->parentRules_.end()) {
    node->parentRules_.erase(ruleIt);
  }
}

Rule* GraphReloader::createNewRule(Rule const* newRule) {
  NodeArray inputs;
  NodeArray outputs;

  Rule* rule = new Rule(inputs, outputs);

  for (auto it = newRule->outputs_.begin();
       it != newRule->outputs_.end(); ++it) {
    auto nodeIt = original_->nodes_.find((*it)->getPath());
    Node* node = nullptr;
    if (nodeIt == original_->nodes_.end()) {
      node = createNewNode(*it);
    } else {
      node = nodeIt->second;
    }

    rule->outputs_.push_back(node);
    node->setChild(rule);
  }

  for (auto it = newRule->inputs_.begin();
       it != newRule->inputs_.end(); ++it) {
    /* Try to find it in the NodeMap of not-already-analyzed Nodes */
    auto nodeIt = originalNodes_.find((*it)->getPath());
    Node* node = nullptr;

    if (nodeIt == originalNodes_.end()) {
      /* if it is not in the map of not-already analyzed, check if this node
       * already exist: */
      nodeIt = original_->nodes_.find((*it)->getPath());
      if (nodeIt == original_->nodes_.end()) {
        /* if this Node is new: create it */
        node = createNewNode(*it);
      } else {
        /* This node exist and was already updated  */
        node = nodeIt->second;
      }
    } else {
      node = nodeIt->second;
      originalNodes_.erase(nodeIt);
    }
    res ret = updateSubGraph(node, *it);
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateNodeHash(*node, ret.updateHash, ret.updateDepsHash);
    }

    rule->inputs_.push_back(node);
    node->addParentRule(rule);

    if (node->isSource() || node->getState() == State::UP_TO_DATE) {
      rule->markInputReady();
    }
  }

  rule->setCommand(newRule->getCommand());
  rule->setDepfile(newRule->getDepfile());

  original_->rules_.push_back(rule);

  hash::updateRuleHash(*rule, true, true);
  rule->setState(State::OUT_OF_DATE);
  return rule;
}

Node* GraphReloader::createNewNode(Node const* newNode) {
  Node* node = new Node(newNode->getPath(), true);

  original_->addNode(node);

  hash::updateNodeHash(*node, true, true);
  watchman_.watchNode(*node);

  return node;
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
