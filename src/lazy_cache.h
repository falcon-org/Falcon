/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_LAZY_CACHE_H_
# define FALCON_LAZY_CACHE_H_

#include "cache_manager.h"
#include "graph.h"

namespace falcon {

class IBuildOutputConsumer;

/**
 * LazyCache is a helper class for performing what we call "lazy cache
 * fetching". The concept is that if you can fetch a target from the cache, you
 * don't need to fetch any sub-target as well, they will be fetched only when
 * needed.
 *
 * TODO: when distributed caching is implemented, we can think of ways to
 * improve efficiency by retrieving multiple targets in one single query.
 */
class LazyCache {
 public:
  LazyCache(NodeSet& targets, CacheManager& cache,
            IBuildOutputConsumer* consumer);

  /** Start a DFS on each node in "targets". When a node is found in cache,
   * retrieve it, mark it up-to-date and stop the DFS. */
  void fetch();

 private:

  void traverseNode(Node* node);
  void traverseRule(Rule* rule);

  /** List of targets we are building. */
  NodeSet& targets_;

  CacheManager& cache_;

  IBuildOutputConsumer* consumer_;
};

} // namespace falcon

#endif // FALCON_LAZY_CACHE_H_
