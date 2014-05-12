/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <cassert>

#include "cache_manager.h"
#include "depfile.h"
#include "exceptions.h"
#include "graph.h"
#include "graph_hash.h"
#include "logging.h"
#include "watchman.h"

namespace falcon {

/* ************************************************************************* */
/*                                Node                                       */
/* ************************************************************************* */

Node::Node(const std::string& path, bool isExpicitDependency)
  : path_(path)
  , hash_()
  , childRule_(nullptr)
  , isExpicitDependency_(isExpicitDependency)
  , state_(State::UP_TO_DATE)
  , timestamp_(0) { }

const std::string& Node::getPath() const { return path_; }

bool Node::isSource() const { return childRule_ == nullptr; }
const Rule* Node::getChild() const { return childRule_; }
Rule*       Node::getChild()       { return childRule_; }

void Node::setChild(Rule* rule) {
  if (childRule_ != nullptr) {
    std::string message = "Invalide Graph -> Node '" + getPath()
                        + "' has already a child";
    THROW_ERROR(EINVAL, message.c_str());
  }
  childRule_ = rule;
}

const RuleArray& Node::getParents() const { return parentRules_; }
RuleArray&       Node::getParents()       { return parentRules_; }

void Node::addParentRule(Rule* rule) { parentRules_.push_back(rule); }
void Node::removeParentRule(Rule* rule) {
  auto it = std::find(parentRules_.begin(), parentRules_.end(), rule);
  assert(it != parentRules_.end());
  parentRules_.erase(it);
}

bool Node::isExplicitDependency() const { return isExpicitDependency_; }

State const& Node::getState() const { return state_; }
State&       Node::getState()       { return state_; }
bool Node::isDirty() const { return state_ == State::OUT_OF_DATE; }

void Node::setState(State state) { state_ = state; }

void Node::markDirty() {
  if (isDirty()) {
    return;
  }

  /* Mark all the parent rules dirty and increase their counter of dirty
   * inputs. */
  for (auto it = parentRules_.begin(); it != parentRules_.end(); ++it) {
    (*it)->markDirty();
    if (!isSource() && !isDirty()) {
      /* If the node is not a source file, increase the counter of inputs
       * that are not ready for the rule. */
      (*it)->markInputDirty();
    }
  }

  DLOG(INFO) << "marking " << path_ << " dirty";
  setState(State::OUT_OF_DATE);
}

Timestamp Node::getTimestamp() const { return timestamp_; }
void Node::setTimestamp(Timestamp t) {
  DLOG(INFO) << "node(" << path_ << ") update timestamp: ("
             << timestamp_ << ") -> (" << t << ")";
  timestamp_ = t;
}

void Node::setHash(std::string const& hash) {
  DLOG(INFO) << "hash(" << getPath() << ") = [" << hash << "]";
  hash_ = hash;
}
std::string const& Node::getHash() const { return hash_; }
std::string& Node::getHash() { return hash_; }

void Rule::setHashDepfile(std::string const& hash) {
  hashDepfile_ = hash;
}
std::string const& Rule::getHashDepfile() const {
  return hashDepfile_;
}
std::string& Rule::getHashDepfile() { return hashDepfile_; }

bool Node::operator==(Node const& n) const { return getPath() == n.getPath(); }
bool Node::operator!=(Node const& n) const { return getPath() != n.getPath(); }

/* ************************************************************************* */
/*                                Rule                                       */
/* ************************************************************************* */

Rule::Rule(const NodeArray& inputs, const NodeArray& outputs)
  : inputs_(inputs)
  , outputs_(outputs)
  , numImplicitDeps_(0)
  , state_(State::UP_TO_DATE)
  , timestamp_(0)
  , numInputsReady_(0)
{ }

const NodeArray& Rule::getInputs() const { return inputs_; }
NodeArray&       Rule::getInputs()       { return inputs_; }
unsigned int Rule::getNumImplicitInputs() const { return numImplicitDeps_;  }
void Rule::setNumImplicitInputs(unsigned int n) { numImplicitDeps_ = n; }
void Rule::addImplicitInput(Node* node) {
  numImplicitDeps_++;
  inputs_.push_back(node);

}
void Rule::addInput(Node* node) {
  /* Explicit inputs should be set up before implicit ones. */
  assert(numImplicitDeps_ == 0);
  inputs_.push_back(node);
}
bool Rule::isInput(const Node* node) const {
  return std::find(inputs_.begin(), inputs_.end(), node) != inputs_.end();
}

const NodeArray& Rule::getOutputs() const { return outputs_; }
NodeArray&       Rule::getOutputs()       { return outputs_; }

bool Rule::isPhony() const { return command_.empty(); }
const std::string& Rule::getCommand() const { return command_; }
void Rule::setCommand(const std::string& cmd) { command_ = cmd; }

const bool Rule::hasDepfile() const { return !depfile_.empty(); }
const std::string& Rule::getDepfile() const { return depfile_; }
void Rule::setDepfile(const std::string& depfile) { depfile_ = depfile; }

State const& Rule::getState() const { return state_; }
State&       Rule::getState()       { return state_; }
bool Rule::isDirty() const { return state_ == State::OUT_OF_DATE; }
void Rule::setState(State state) { state_ = state; }

void Rule::markDirty() {
  if (isDirty()) {
    return;
  }

  /* Mark all the outputs dirty. */
  for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
    (*it)->markDirty();
  }

  setState(State::OUT_OF_DATE);
}

void Rule::setHash(std::string const& hash) { hash_ = hash; }
std::string const& Rule::getHash() const { return hash_; }
std::string& Rule::getHash() { return hash_; }

void Node::setHashDepfile(std::string const& hash) {
  hashDepfile_ = hash;
}
std::string const& Node::getHashDepfile() const {
  return hashDepfile_;
}
std::string& Node::getHashDepfile() { return hashDepfile_; }

Timestamp Rule::getTimestamp() const { return timestamp_; }
void Rule::setTimestamp(Timestamp t) { timestamp_ = t; }

bool Rule::ready() const { return numInputsReady_ == inputs_.size(); }
size_t Rule::numReady() const { return numInputsReady_; }
void Rule::markInputReady() {
  numInputsReady_++; assert(numInputsReady_ <= inputs_.size());
}
void Rule::markInputDirty() { assert(numInputsReady_ > 0); numInputsReady_--; }

/* ************************************************************************* */
/*                                Graph                                      */
/* ************************************************************************* */

Graph::Graph() {}

void Graph::addNode(Node* node) {
  if (node->getParents().empty()) {
    roots_.insert(node);
  }

  if (node->isSource()) {
    sources_.insert(node);
  }

  if (nodes_.find(node->getPath()) != nodes_.end()) {
    std::string message = "Invalid Graph -> Node '" + node->getPath()
                        + "' is already present";
    THROW_ERROR(EINVAL, message.c_str());
  }
  nodes_[node->getPath()] = node;
}

Graph::~Graph() {
  for ( ; !rules_.empty(); rules_.pop_back()) {
    Rule* it = rules_.back();
    delete it;
  }

  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    delete it->second;
  }
}

const NodeSet& Graph::getRoots() const { return roots_; }
NodeSet& Graph::getRoots() { return roots_; }

const NodeSet& Graph::getSources() const { return sources_; }
NodeSet& Graph::getSources() { return sources_; }

const NodeMap& Graph::getNodes() const { return nodes_; }
NodeMap& Graph::getNodes() { return nodes_; }

const RuleArray& Graph::getRules() const { return rules_; }
RuleArray& Graph::getRules() { return rules_; }

} // namespace falcon
