/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_DEPENDENCY_SCAN_H_
#define FALCON_GRAPH_DEPENDENCY_SCAN_H_

#include "graph.h"

namespace falcon {

class CacheManager;

/** GraphDependencyScan is used to traverse an entire graph and detect which
 * Nodes and Rules are dirty by stat'ing the files and comparing the timestamps
 * of the outputs of a Rule against the timestamps of the inputs.
 *
 * This is called in three situations:
 * - On startup. Even though we can detect which targets become dirty at
 *   run-time with watchman, we still need to traverse the entire graph on
 *   startup;
 * - When the graph is reloaded;
 * - When watchman fails: we fall back to using this method. */
class GraphDependencyScan {
 public:
  GraphDependencyScan(Graph& graph, CacheManager* cache);
  void scan();

 private:

  /**
   * @param r Rule for which to retrieve the oldest output;
   * @return Pointer to the oldest output of the rule.
   */
  static Node* getOldestOutput(Rule *r);

  /**
   * Compare all the outputs of a rule with its inputs.
   * If an input is found to be newer than an output, and this input is a source
   * file, it is marked dirty.
   * @param r Rule for which to compare the outputs with the inputs.
   * @return true if at least one input is newer than one input.
   */
  static bool compareInputsWithOutputs(Rule *r);

  /** Stat the given node and update its timestamp. */
  void statNode(Node* node);

  /**
   * Traverse a Node.
   * @param node node to traverse.
   * @return True if the node was marked dirty.
   */
  bool updateNode(Node* node);

  /**
   * Traverse a Rule.
   * @param rule rule to traverse.
   * @return True if the rule was marked dirty.
   */
  bool updateRule(Rule* rule);

  /**
   * Load the depfile of a rule. This will query the cache for the depfile if
   * not found.
   * @param r Rule for which to load depfiles.
   * @return true if the depfile was succesfully loaded.
   */
  bool ruleLoadDepfile(Rule* r);

  Graph& graph_;
  RuleSet seen_;
  CacheManager* cache_;
};

} // namespace falcon

#endif // FALCON_GRAPH_DEPENDENCY_SCAN
