/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_DEPENDENCY_SCAN_H_
#define FALCON_GRAPH_DEPENDENCY_SCAN_H_

#include "graph.h"

namespace falcon {

class CacheManager;

class GraphDependencyScan {
 public:
  GraphDependencyScan(Graph& graph, CacheManager* cache);
  void scan();

 private:

  static Node* getOldestOutput(Rule *r);
  static bool compareInputsWithOutputs(Rule *r);
  void statNode(Node* node);

  bool updateNode(Node* node);
  bool updateRule(Rule* rule);

  Graph& graph_;
  RuleSet seen_;
  CacheManager* cache_;
};

} // namespace falcon

#endif // FALCON_GRAPH_DEPENDENCY_SCAN
