/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_SEQUENTIAL_BUILDER_H_
#define FALCON_GRAPH_SEQUENTIAL_BUILDER_H_

#include <queue>

#include "graph.h"

namespace falcon {

class GraphSequentialBuilder {
 public:
  GraphSequentialBuilder(Graph& graph);

  /**
   * Build the given targets.
   * @param targets Targets to build.
   * @return True if build of all the targets succeeded.
   */
  bool build(NodeSet& targets);

 private:

  /**
   * Build the given target.
   * @param target Target to build.
   * @return True if build of the target succeeded.
   */
  bool buildTarget(Node* target);

  Graph& graph_;
};

} // namespace falcon

#endif // FALCON_GRAPH_SEQUENTIAL_BUILDER_H_

