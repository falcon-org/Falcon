/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph.h"

namespace falcon {

Node::Node(const std::string& path)
 : path_(path) { }

const std::string& Node::getPath() const {
  return path_;
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

} // namespace falcon
