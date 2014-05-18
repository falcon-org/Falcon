/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "lazy_cache.h"

#include "logging.h"
#include "stream_server.h"

namespace falcon {

LazyCache::LazyCache(NodeSet& targets, CacheManager& cache,
                     IBuildOutputConsumer* consumer)
    : targets_(targets)
    , cache_(cache)
    , consumer_(consumer) { }

void LazyCache::fetch() {
  for (auto it = targets_.begin(); it != targets_.end(); ++it) {
    traverseNode(*it);
  }
}

void LazyCache::traverseNode(Node* node) {
  if (!node->isDirty()) {
    /* This node is up-to-date. */
    return;
  }

  Rule* rule = node->getChild();
  if (!rule) {
    /* This is a source file. Ignore it. */
    return;
  }

  if (rule->isPhony()) {
    /* This is the output of a phony rule. */
    traverseRule(rule);
    return;
  }

  if (cache_.restoreNode(node)) {
    consumer_->cacheRetrieveAction(node->getPath());
    node->setState(State::UP_TO_DATE);
    node->setLazyFetched(true);
    /* Update the timestamp of the node. This will make sure that we don't mark
     * it dirty when watchman notifies us it changed. */
    node->setTimestamp(time(NULL));
    /* Notify the parents of this output that one of their inputs is ready. */
    auto parentRules = node->getParents();
    for (auto it = parentRules.begin(); it != parentRules.end(); ++it) {
      (*it)->markInputReady();
    }
  } else {
    /* We could not restore the node, go deeper. */
    traverseRule(rule);
  }

}

void LazyCache::traverseRule(Rule* rule) {
  auto& inputs = rule->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    traverseNode(*it);
  }
}

} // namespace falcon
