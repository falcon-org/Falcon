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

/* Helper to find a Node in the given NodeArray */
Node* GraphReloader::getNodeFromSource(NodeArray& source, Node* newNode) {
  Node* node = nullptr;

  auto nodeIt = std::find_if(source.begin(), source.end(),
      [newNode](Node* n) { return newNode->getPath() == n->getPath(); }
    );

  if (nodeIt != source.end()) {
    node = *nodeIt;
    /* Erase it from the given NodeArray */
    source.erase(nodeIt);
    /* Erase the occurence from the originalNodes_ (remember the remaining
     * nodes in this array will be deleted at the end) */
    originalNodes_.erase(node->getPath());
  }

  return node;
}

/* Helper to find a Node from:
 *  -- originalNodes_ (the node comes from the original graph and hasn't been
 *                     visited yet)
 *  or original_ (the node has been visited or created and already exist in the
 *                updated graph)
 *  or create it */
Node* GraphReloader::getNodeFromAll(Node* newNode) {
  Node* node = nullptr;

  auto nodeItPair = originalNodes_.find(newNode->getPath());
  if (nodeItPair == originalNodes_.end()) {
    nodeItPair = original_->nodes_.find(newNode->getPath());

    if (nodeItPair == originalNodes_.end()) {
      node = createNewNode(newNode);
    } else {
      node = nodeItPair->second;
    }
  } else {
    node = nodeItPair->second;
    originalNodes_.erase(nodeItPair);
  }

  return node;
}

void GraphReloader::updateRoots() {
  originalNodes_ = original_->nodes_;
  original_->nodes_.clear();
  original_->roots_.clear();
  original_->sources_.clear();

  for (auto it = new_->getRoots().cbegin();
       it != new_->getRoots().cend();
       ++it) {
    Node* root = getNodeFromAll(*it);

    DLOG(INFO) << "Update graph root: " << root->getPath();

    res ret = updateSubGraph(root, *it);
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateNodeHash(*root, ret.updateHash, ret.updateDepsHash);
      root->setState(State::OUT_OF_DATE);
    }

    original_->roots_.insert(root);
  }

  /* Delete no longer needed nodes */
  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    assert(it->second->parentRules_.empty());

    /* request watchman stop to wath it */
    watchman_.unwatchNode(*(it->second));

    if (!it->second->isSource()) {
      deleteChildRule(it->second);
      /* TODO: At this point, the output can be deleted?
       * -1- unlink the file or remove the directory ?
       * -2- if it is a socket ? close it and remove it ?
       * -3- if it is a symbolic link ?
       * -4- a named pipe ? */
    }

    original_->nodes_.erase(it->first);
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

  if (node->isSource() && newNode->isSource()) {
    DLOG(INFO) << "  node '" << node->getPath() << "' is a source file";
    /* Nothing to change here, the node still a source file
     * just insert it in the Sources Set */
    original_->sources_.insert(node);
  } else if (!node->isSource() && newNode->isSource()) {
    DLOG(INFO) << "  node '" << node->getPath() << "' is now a source file";
    /* The node is now a source */
    deleteChildRule(node);
    statNode(node);
    original_->sources_.insert(node);
    r.updateHash = true;
  } else { /* !newNode->isSource() */
    if (node->isSource()) {
      DLOG(INFO) << "  node '" << node->getPath() << "' is now an output";
      /* The Node used to be a Source file. But is now an output of a rule.
       * Create it: */
      Rule* rule = createNewRule(newNode->getChild(), node);
      original_->rules_.push_back(rule);
    }
    /* Update the sub rule */
    res ret = updateSubGraph(node->getChild(), newNode->getChild());
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateRuleHash(*node->getChild(), ret.updateHash, ret.updateDepsHash);
    }
    r.updateHash = ret.updateHash;
    r.updateDepsHash = ret.updateDepsHash;
  }

  if (r.updateHash || r.updateDepsHash) {
    node->setState(State::OUT_OF_DATE);
  }

  original_->nodes_[node->getPath()] = node;
  return r;
}


/* Even if there is no modification, we un-build and then re-build the rule */
GraphReloader::res
GraphReloader::updateSubGraph(Rule* rule, Rule const* newRule) {
  res r = { .updateHash = false, .updateDepsHash = false };

  NodeArray inputs(rule->inputs_.begin(),
                   rule->inputs_.end() - rule->numImplicitDeps_);
  NodeArray implicitDepsBefore(rule->inputs_.end() - rule->numImplicitDeps_,
                               rule->inputs_.end());

  rule->inputs_.clear(); /* Clear inputs */
  rule->numInputsReady_ = 0; /* no input, so no input ready */

  /* Build inputs */
  for (auto it = newRule->inputs_.begin(); it != newRule->inputs_.end(); ++it) {
    Node* node = nullptr;

    /* look if the node comes from the original rule */
    node = getNodeFromSource(inputs, *it);
    if (node == nullptr) {
      /* The node wasn't in the original rule's input:
       * So this is a new node (at least from the rule point of view)
       *
       * The rule has been changed: we need to update the hash */
      r.updateHash = true;

      /* Get it from the the already existing Node or create it */
      node = getNodeFromAll(*it);

      /* Since the node wasn't yet in the rule's inputs, we need to add the
       * rule in its parent rules */
      node->addParentRule(rule);
    }

    assert(node->isExplicitDependency());

    /* Update the input subgraph */
    res ret = updateSubGraph(node, *it);
    if (ret.updateHash || ret.updateDepsHash) {
      hash::updateNodeHash(*node, ret.updateHash, ret.updateDepsHash);
    }
    r.updateHash |= ret.updateHash;
    r.updateDepsHash |= ret.updateDepsHash;

    rule->inputs_.push_back(node);
    if (node->isSource() || node->getState() == State::UP_TO_DATE) {
      rule->markInputReady();
    }
  }
  /* Remove no longer needed inputs */
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    deleteParentRule(*it, rule);
    clearSubGraph(*it);
    /* At this point, we don't need to really delete the node:
     * the node may be needed in future and if not it will be deleted at the
     * end */
  }

  NodeArray outputs(rule->outputs_); /* Save the outputs */
  rule->outputs_.clear();

  /* build the rule's outputs */
  for (auto it = newRule->outputs_.begin(); it != newRule->outputs_.end(); ++it) {
    Node* node = nullptr;

    /* get it from the original output */
    node = getNodeFromSource(outputs, *it);
    if (node == nullptr) {
      /* This is a new output (at least from the rule point of view):
       * So the rule has been changed: we will need to update the hash: */
      r.updateHash = true;

      /* Get it from the already existing Node or create it */
      node = getNodeFromAll(*it);

      /* Since the node wasn't yet in the rule inputs, we need to add the
       * rule in its parent rules */
      node->addParentRule(rule);
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

  if (rule->getCommand() != newRule->getCommand()) {
    rule->setCommand(newRule->getCommand());
    r.updateHash = true;
    r.updateDepsHash = true;
  }

  /* Rebuild depfile */
  if (rule->getDepfile() != newRule->getDepfile()) {
    /* This is a new depfile:
     * so: reset the depfile, and clear the implicitDeps */
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
    /* The depfile remains the same: re-insert the implicitDeps. */
    for (auto it = implicitDepsBefore.begin(); it != implicitDepsBefore.end(); ++it) {
      assert(!(*it)->isExplicitDependency());
      rule->inputs_.push_back(*it);
      originalNodes_.erase((*it)->getPath());
      original_->nodes_[(*it)->getPath()] = *it;
      if ((*it)->isSource() || (*it)->getState() == State::UP_TO_DATE) {
        rule->markInputReady();
      }
    }
  }

  /* If the rule has been changed, mark this rule dirty */
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

Rule* GraphReloader::createNewRule(Rule const* newRule, Node* output) {
  NodeArray inputs;
  NodeArray outputs;

  Rule* rule = new Rule(inputs, outputs);

  rule->outputs_.push_back(output);
  output->setChild(rule);

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
