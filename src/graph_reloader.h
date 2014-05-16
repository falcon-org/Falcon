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


class GraphReloader {
public:
  GraphReloader(Graph* original, Graph* newGraph, WatchmanClient& watchman,
                CacheManager* cache);
  ~GraphReloader();

  Graph* getUpdatedGraph();

private:
  Graph* original_;
  Graph* new_;
  WatchmanClient& watchman_;
  CacheManager* cache_;

private:

  typedef struct {
    bool updateHash;
    bool updateDepsHash;
  } res;

  void updateRoots();

  res updateSubGraph(Node* node, Node const* newNode);
  res updateSubGraph(Rule* rule, Rule const* newRule);

  Rule* createNewRule(Rule const* newRule);
  void deleteChildRule(Node* node);
  void deleteParentRule(Node* node, Rule const* rule);

  Node* createNewNode(Node const* newNode);
  void clearSubGraph(Node* n);

private:
  NodeMap originalNodes_;
  NodeSet nodesSeen_;
};

}

#endif /* !FALCON_GRAPH_RELOADER_H_ */
