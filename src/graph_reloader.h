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
  GraphReloader(Graph* original, Graph* newGraph,
                WatchmanClient& watchman, CacheManager* cache);
  ~GraphReloader();

  Graph* getUpdatedGraph();

private:
  Graph* original_;
  Graph* new_;
  WatchmanClient& watchman_;
  CacheManager* cache_;

private:
  /* Use to know when it is relevent to update the Node/Rule hashes */
  typedef struct {
    bool updateHash;
    bool updateDepsHash;
  } res;

  /* Main methods ********************************************************** */
  /* Update the Graph from the roots to the sources (using a DFS) */
  void updateRoots();

  res updateSubGraph(Node* node, Node const* newNode);
  res updateSubGraph(Rule* rule, Rule const* newRule);

  /* Helpers *************************************************************** */
  /* Rules management */
  Rule* createNewRule(Rule const* newRule, Node* output);
  void deleteChildRule(Node* node);
  void deleteParentRule(Node* node, Rule const* rule);

  /* Node management */
  Node* createNewNode(Node const* newNode);
  void clearSubGraph(Node* n);

  /* try to get a Node from the given source NodeArray
   * @return nullptr if not found */
  Node* getNodeFromSource(NodeArray& source, Node* newNode);
  /* get a node in all of the already used Node or create it
   * @return a Node (can't failed) */
  Node* getNodeFromAll(Node* newNode);

private:
  /* Keep a map of the original nodes (the nodes before updating the Graph)
   * all the remaining nodes will be deleted at the end */
  NodeMap originalNodes_;
  /* A set of visited nodes in order to avoid visting more than once a Node */
  NodeSet nodesSeen_;
};

}

#endif /* !FALCON_GRAPH_RELOADER_H_ */
