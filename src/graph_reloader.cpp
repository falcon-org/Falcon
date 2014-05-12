/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_reloader.h"
#include "cache_manager.h"
#include "graph_hash.h"
#include "graph_dependency_scan.h"
#include "logging.h"

#include <cassert>
#include <algorithm>

namespace falcon {

GraphReloader::GraphReloader(Graph* original, Graph* newGraph,
                             WatchmanClient& watchman,
                             CacheManager* cache)
  : original_(original)
  , new_(newGraph)
  , watchman_(watchman)
  , cache_(cache)
{
}

GraphReloader::~GraphReloader() {
  delete original_;
}

Graph* GraphReloader::getUpdatedGraph() {
  watchman_.unwatchGraph(*original_);

  /* Scan the graph to discover what needs to be rebuilt, and compute the
   * hashes of all nodes. */
  falcon::GraphDependencyScan scanner(*new_, cache_);
  scanner.scan();

  watchman_.watchGraph(*new_);

  return new_;
}

}
