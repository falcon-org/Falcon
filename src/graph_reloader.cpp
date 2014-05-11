/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_reloader.h"
#include "graph_hash.h"
#include "graph_dependency_scan.h"
#include "logging.h"

#include <cassert>
#include <algorithm>

namespace falcon {

GraphReloader::GraphReloader(Graph* original, Graph* newGraph,
                             WatchmanClient& watchman)
  : original_(original)
  , new_(newGraph)
  , watchman_(watchman)
{
}

GraphReloader::~GraphReloader() {
  delete original_;
}

Graph* GraphReloader::getUpdatedGraph() {
  /* Scan the graph to discover what needs to be rebuilt, and compute the
   * hashes of all nodes. */
  falcon::GraphDependencyScan scanner(*new_);
  scanner.scan();

  watchman_.watchGraph(*new_);

  return new_;
}

}
