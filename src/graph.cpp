/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <cassert>

#include "graph.h"
#include "logging.h"

namespace falcon {

/* ************************************************************************* */
/*                                Node                                       */
/* ************************************************************************* */

Node::Node(const std::string& path)
 : path_(path)
 , childRule_(nullptr)
 , state_(State::OUT_OF_DATE)
 , newTimeStamp_(0)
 , oldTimeStamp_(0) { }

const std::string& Node::getPath() const { return path_; }

const Rule* Node::getChild() const { return childRule_; }
Rule*       Node::getChild()       { return childRule_; }

void Node::setChild(Rule* rule) {
  assert(childRule_ == nullptr);
  childRule_ = rule;
}

const RuleArray& Node::getParents() const { return parentRules_; }
RuleArray&       Node::getParents()       { return parentRules_; }

void Node::addParentRule(Rule* rule) { parentRules_.push_back(rule); }

State const& Node::getState() const { return state_; }
State&       Node::getState()       { return state_; }
bool Node::isDirty() const { return state_ == State::OUT_OF_DATE; }

void Node::setState(State state) { state_ = state; }

void Node::markDirty() {
  LOG(trace) << "marking " << path_ << " dirty";

  if (state_ == State::OUT_OF_DATE) {
    /* This node is already dirty. */
    LOG(trace) << path_ << " is already dirty";
    return;
  }
  state_ = State::OUT_OF_DATE;

  /* Mark all the parent rules dirty. */
  for (auto it = parentRules_.begin(); it != parentRules_.end(); ++it) {
    (*it)->markDirty();
  }
}
void Node::markUpToDate() {
  if (state_ == State::UP_TO_DATE) {
    return;
  }
  state_ = State::UP_TO_DATE;

  /* Mark all the parent rules up to date. */
  for (auto it = parentRules_.begin(); it != parentRules_.end(); ++it) {
    (*it)->markUpToDate();
  }
}

TimeStamp Node::getTimeStamp() const { return newTimeStamp_; }
TimeStamp Node::getPreviousTimeStamp() const { return oldTimeStamp_; }
void Node::updateTimeStamp(TimeStamp const t) {
  oldTimeStamp_ = newTimeStamp_;
  newTimeStamp_ = t;
  LOG(trace) << "node(" << path_ << ") update timestamp: ("
             << oldTimeStamp_ << ") -> (" << newTimeStamp_ << ")";
}

bool Node::operator==(Node const& n) const { return getPath() == n.getPath(); }
bool Node::operator!=(Node const& n) const { return getPath() != n.getPath(); }

void Node::accept(GraphVisitor& v) { v.visit(*this); }

/* ************************************************************************* */
/*                                Rule                                       */
/* ************************************************************************* */

Rule::Rule(const NodeArray& inputs, const NodeArray& outputs)
  : inputs_(inputs)
  , outputs_(outputs)
  , state_(State::OUT_OF_DATE) { }

const NodeArray& Rule::getInputs() const { return inputs_; }
NodeArray&       Rule::getInputs()       { return inputs_; }
void Rule::addInput(Node* node) { inputs_.push_back(node); }
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
  if (state_ == State::OUT_OF_DATE) {
    /* This rule is already dirty. */
    return;
  }

  state_ = State::OUT_OF_DATE;

  /* Mark all the outputs dirty. */
  for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
    (*it)->markDirty();
  }
}
void Rule::markUpToDate() {
  if (state_ == State::UP_TO_DATE) {
    return;
  }

  state_ = State::UP_TO_DATE;

  /* Mark all the outputs UpToDate. */
  for (auto it = outputs_.begin(); it != outputs_.end(); ++it) {
    (*it)->markUpToDate();
  }
}

void Rule::accept(GraphVisitor& v) {
  v.visit(*this);
}

/* ************************************************************************* */
/*                                Graph                                      */
/* ************************************************************************* */

Graph::Graph(const NodeSet& roots, const NodeSet& sources,
             const NodeMap& nodes, const RuleArray& rules)
    : roots_(roots)
    , sources_(sources)
    , nodes_(nodes)
    , rules_(rules) {}

void Graph::addNode(Node* node) {
  if (node->getParents().empty()) {
    roots_.insert(node);
  }

  if (node->getChild() == nullptr) {
    sources_.insert(node);
  }

  assert(nodes_.find(node->getPath()) == nodes_.end());
  nodes_[node->getPath()] = node;
}

const NodeSet& Graph::getRoots() const { return roots_; }
NodeSet& Graph::getRoots() { return roots_; }

const NodeSet& Graph::getSources() const { return sources_; }
NodeSet& Graph::getSources() { return sources_; }

const NodeMap& Graph::getNodes() const { return nodes_; }
NodeMap& Graph::getNodes() { return nodes_; }

const RuleArray& Graph::getRules() const { return rules_; }
RuleArray& Graph::getRules() { return rules_; }

void Graph::accept(GraphVisitor& v) { v.visit(*this); }

} // namespace falcon
