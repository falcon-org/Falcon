/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_RELOADER_H_
# define FALCON_GRAPH_RELOADER_H_

# include "graph.h"
# include "watchman.h"

namespace falcon {

class CacheManager;

/*!
 * @class GraphReloader
 * @brief Reload a graph: update every information and save the maximum of the
 * information from the previous state. */
class GraphReloader {
public:
  GraphReloader(Graph& original, Graph& newGraph, WatchmanClient& watchman);

  void updateGraph();

private:
  Graph& original_;
  Graph& new_;
  WatchmanClient& watchman_;

private:
  /* Main methods ********************************************************** */
  /* Update the Graph from the roots to the sources (using a DFS) */
  void updateRoots();

  /* remove no longer needed nodes */
  void cleanStaleSubgraph();

  bool updateSubGraph(Node* node, Node const* newNode);
  bool updateSubGraph(Rule* rule, Rule const* newRule);

  bool updateRuleInputs(Rule* rule, NodeArray& inputs, Rule const* newRule);
  bool updateRuleOutputs(Rule* rule, Rule const* newRule);
  bool updateRuleDepfiles(Rule* rule, NodeArray& implicitDepsBefore,
                          Rule const* newRule);

  /* Helpers *************************************************************** */
  /* Rules management */
  Rule* createNewRule(Rule const* newRule, Node* output);
  void deleteChildRule(Node* node);

  /* Node management */
  void clearSubGraph(Node* n);

  /* get a node from all of the already used Nodes or create it
   * @return a Node (can't failed) */
  Node* getNodeFromAll(Node* newNode);

  std::pair<Node*, bool> getNode(NodeArray& source, Node* newNode);

private:
  /* Keep a map of the original nodes (the nodes before updating the Graph)
   * all the remaining nodes will be deleted at the end */
  NodeMap originalNodes_;
  /* A set of visited nodes in order to avoid visting more than once a Node */
  NodeSet nodesSeen_;
};

}

#endif /* !FALCON_GRAPH_RELOADER_H_ */
