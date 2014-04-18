/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_H_
#define FALCON_GRAPH_H_

#include <string>
#include <vector>

/** This file defines the data structure for storing the graph of nodes and
 * commands. */

namespace falcon {

/** Class that represents a node in the graph. */
class Node {
 public:
  explicit Node(const std::string& path);

  const std::string& getPath() const;

 private:
  std::string path_;

  Node(const Node& other) = delete;
  Node& operator=(const Node&) = delete;
};

typedef std::vector<Node*> NodeArray;

/** Class that represents a rule in the graph.
 * A rule is a link between input nodes and output nodes. */
class Rule {
 public:
  /**
   * Construct a rule.
   * @param inputs  Inputs of the rule.
   * @param outputs Outputs of the rule.
   * @param cmd     Command to be executed in order to generate the outputs.
   */
  explicit Rule(const NodeArray& inputs, const NodeArray& outputs,
                const std::string& cmd);

  /**
   * Construct a phony rule.
   * @param inputs  Inputs of the rule.
   * @param output  Output of the rule. For a phony rule, there is only one
   *                output.
   */
  explicit Rule(const NodeArray& outputs, Node* output);

  const NodeArray& getInputs() const;
  const NodeArray& getOutputs() const;
  bool isPhony() const;
  const std::string& getCommand() const;

 private:
  NodeArray inputs_;
  NodeArray outputs_;

  /* Command to execute in order to generate the outputs. All the inputs must be
   * up-to-date prior to executing the command.
   * Empty string is this is a phony rule. */
  std::string command_;

  /* Set to true if this is a phony rule. A phony rule has no command. */
  bool isPhony_;

  Rule(const Rule& other) = delete;
  Rule& operator=(const Rule&) = delete;
};

} // namespace falcon

#endif // FALCON_GRAPH_H_
