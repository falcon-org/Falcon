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

GraphReloader::GraphReloader(Graph* original, Graph* newGraph,
                             WatchmanClient& watchman,
                             CacheManager* cache)
  : original_(original)
  , new_(newGraph)
  , watchman_(watchman)
  , cache_(cache)
{
}

GraphReloader::~GraphReloader() {
  delete original_;
}

Graph* GraphReloader::getUpdatedGraph() {
  return original_;
}

void GraphReloader::updateRoots() {
  /* Rebuild the Graph from the roots: */
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
      /* Remove it from the originalNodes */
      root = rootIt->second;
      originalNodes_.erase(rootIt);
    }
    if (updateSubGraph(root, *it)) {
      hash::updateNodeHash(*root, true, false);
    }

    original_->roots_.insert(root);
  }

  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    if (!it->second->isSource()) {
      deleteChildRule(it->second);
    }
    original_->nodes_.erase(it->first);
    /* TODO: request watchman stop to wath it */
  }
  for (auto it = originalNodes_.begin(); it != originalNodes_.end(); ++it) {
    delete it->second;
    DLOG(INFO) << "delete node: " << it->first;
  }

  DLOG(INFO) << "### LOOK AT THE RULES";
  for (auto it = original_->rules_.begin();
       it != original_->rules_.end(); ++it) {
    Rule* rule = *it;
    DLOG(INFO) << "# check Rule: cmd(" << rule->getCommand().substr(0, 32)
               << ") input(" << rule->inputs_[0]->getPath() << ")";
    if (GraphDependencyScan::compareInputsWithOutputs(rule) &&
        !rule->isPhony()) {
      rule->markDirty();
    }
  }

  DLOG(INFO) << "##############The Graph has been reloaded !!!!";
}

bool GraphReloader::updateSubGraph(Node* node, Node const* newNode) {
  if (nodesSeen_.find(node) != nodesSeen_.end()) {
    return false;
  }
  nodesSeen_.insert(node);

  DLOG(INFO) << "update Node(" << node->getPath() << ")";

  bool needToUpdateHash = false;

  if (node->isSource() && newNode->isSource()) {
    /* Nothing to change here, the node still a source file
     * just insert it in the Graph sources set */
    original_->sources_.insert(node);
  } else if (node->isSource() && !newNode->isSource()) {
    /* The Node used to be a Source file. But is now an output of a rule.
     * Create it: */
    createNewRule(newNode->getChild());
    needToUpdateHash = true;
  } else if (!node->isSource() && newNode->isSource()) {
    /* The node is now a source */
    deleteChildRule(node);
    statNode(node);
    needToUpdateHash = true;
    original_->sources_.insert(node);
  } else {
    /* Update the sub rule */
    if (updateSubGraph(node->getChild(), newNode->getChild())) {
      needToUpdateHash = true;
      hash::updateRuleHash(*node->getChild(), false, false);
    }
  }

  return needToUpdateHash;
}

bool GraphReloader::updateSubGraph(Rule* rule, Rule const* newRule) {
  bool needToUpdateHash = false;
  bool keepDepFile = false;

  NodeArray inputs(rule->inputs_);
  if (rule->getDepfile() == newRule->getDepfile()) {
    keepDepFile = true;
  }

  rule->inputs_.clear();

  for (auto it = newRule->inputs_.begin(); it != newRule->inputs_.end(); ++it) {
    /* First, try to find it in the current input */
    auto nodeIt = std::find_if(inputs.begin(), inputs.end(),
        [it](Node* n) { return (*it)->getPath() == n->getPath(); }
      );
    Node* node;
    if (nodeIt == inputs.end()) {
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
    } else {
      node = *nodeIt;
      inputs.erase(nodeIt);
      originalNodes_.erase(node->getPath());
    }
    if (updateSubGraph(node, *it)) {
      hash::updateNodeHash(*node, true, false);
      needToUpdateHash = true;
    }
    DLOG(INFO) << "add input node: " << node->getPath();;
    rule->inputs_.push_back(node);
    node->addParentRule(rule);
  }
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    Node* node = *it;
    if (!keepDepFile || node->isExplicitDependency()) {
      DLOG(INFO) << "delete parent rule of " << node->getPath();
      deleteParentRule(node, rule);
    } else {
      rule->inputs_.push_back(node);
      node->addParentRule(rule);
      originalNodes_.erase(node->getPath());
    }
  }

  if (!keepDepFile) {
    rule->setDepfile(newRule->getDepfile());
    auto res = Depfile::loadFromfile(rule->getDepfile(), rule,
                                     nullptr, *original_,
                                     false);
    needToUpdateHash |= res != Depfile::Res::SUCCESS;
  }

  rule->numInputsReady_ = rule->inputs_.size();
  for (auto it = rule->inputs_.begin(); it != rule->inputs_.end(); ++it) {
    if ((*it)->isDirty()) {
      rule->numInputsReady_--;
    }
  }

  NodeArray outputs(rule->outputs_);
  rule->outputs_.clear();

  for (auto it = newRule->outputs_.begin(); it != newRule->outputs_.end(); ++it) {
    /* First, try to find it in the current outputs */
    auto nodeIt = std::find_if(outputs.begin(), outputs.end(),
        [it](Node* n) { return (*it)->getPath() == n->getPath(); }
      );
    Node* node;
    if (nodeIt == outputs.end()) {
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
    } else {
      node = *nodeIt;
      outputs.erase(nodeIt);
    }
    DLOG(INFO) << "add output node: " << node->getPath();;
    rule->outputs_.push_back(node);
  }
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    Node* node = outputs.back();
    deleteChildRule(node);
    DLOG(INFO) << "delete child rule of " << node->getPath();
  }

  if (rule->getCommand() != newRule->getCommand()) {
    rule->setCommand(newRule->getCommand());
    needToUpdateHash = true;
  }

  return needToUpdateHash;
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
    if (updateSubGraph(node, *it)) {
      hash::updateNodeHash(*node, true, false);
    }

    rule->inputs_.push_back(node);
    node->addParentRule(rule);
  }

  rule->setCommand(newRule->getCommand());
  rule->setDepfile(newRule->getDepfile());
  /* TODO: load the depfile ?? */

  original_->rules_.push_back(rule);

  hash::updateRuleHash(*rule, true, false);
  return rule;
}

Node* GraphReloader::createNewNode(Node const* newNode) {
  Node* node = new Node(newNode->getPath(), false);

  original_->addNode(node);

  /* Scan the graph to discover what needs to be rebuilt, and compute the
   * hashes of all nodes. */
  falcon::GraphDependencyScan scanner(*new_, cache_);
  scanner.scan();

  hash::updateNodeHash(*node, true, false);
  watchman_.watchNode(*node);

  return new_;
}

}
