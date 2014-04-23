/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>

#include "graph.h"

namespace falcon {

/* ************************************************************************* */
/*                                Node                                       */
/* ************************************************************************* */

Node::Node(const std::string& path)
 : path_(path)
 , childRule_(nullptr)
 , state_(State::OUT_OF_DATE) { }

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

void Node::setState(State state) { state_ = state; }

void Node::markDirty() {
  if (state_ == State::OUT_OF_DATE) {
    /* This node is already dirty. */
    return;
  }
  state_ = State::OUT_OF_DATE;

  /* Mark all the parent rules dirty. */
  for (auto it = parentRules_.begin(); it != parentRules_.end(); ++it) {
    (*it)->markDirty();
  }
}

bool Node::operator==(Node const& n) const { return getPath() == n.getPath(); }
bool Node::operator!=(Node const& n) const { return getPath() != n.getPath(); }

void Node::accept(GraphVisitor& v) { v.visit(*this); }

/* ************************************************************************* */
/*                                Rule                                       */
/* ************************************************************************* */

Rule::Rule(const NodeArray& inputs, const NodeArray& outputs,
           const std::string& cmd)
  : inputs_(inputs)
  , outputs_(outputs)
  , command_(cmd)
  , isPhony_(false)
  , state_(State::OUT_OF_DATE) { }

Rule::Rule(const NodeArray& inputs, Node* output)
    : inputs_(inputs)
    , outputs_(1, output)
    , isPhony_(true)
    , state_(State::OUT_OF_DATE) { }

const NodeArray& Rule::getInputs() const { return inputs_; }
NodeArray&       Rule::getInputs()       { return inputs_; }

const NodeArray& Rule::getOutputs() const { return outputs_; }
NodeArray&       Rule::getOutputs()       { return outputs_; }

bool Rule::isPhony() const { return isPhony_; }

const std::string& Rule::getCommand() const { return command_; }

State const& Rule::getState() const { return state_; }
State&       Rule::getState()       { return state_; }
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
