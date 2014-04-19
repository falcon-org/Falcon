/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>

#include "graph.h"

namespace falcon {

Node::Node(const std::string& path)
 : path_(path) { }

const std::string& Node::getPath() const {
  return path_;
}

void Node::setChild(Rule* rule) {
  assert(childRule_ == nullptr);
  childRule_ = rule;
}

void Node::addParentRule(Rule* rule) {
  parentRules_.push_back(rule);
}

const NodeArray& Rule::getInputs() const {
  return inputs_;
}

const NodeArray& Rule::getOutputs() const {
  return outputs_;
}

Rule::Rule(const NodeArray& inputs, const NodeArray& outputs,
           const std::string& cmd)
  : inputs_(inputs), outputs_(outputs), command_(cmd), isPhony_(false) { }

Rule::Rule(const NodeArray& inputs, Node* output)
    : inputs_(inputs), outputs_(1, output), isPhony_(true) { }

bool Rule::isPhony() const {
  return isPhony_;
}

const std::string& Rule::getCommand() const {
  return command_;
}

Graph::Graph(const NodeSet& roots, const NodeMap& nodes)
    : roots_(roots), nodes_(nodes) {}

const NodeSet& Graph::getRoots() const {
  return roots_;
}

const NodeMap& Graph::getNodes() const {
  return nodes_;
}

} // namespace falcon
