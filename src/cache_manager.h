/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_CACHE_MANAGER_H_
#define FALCON_CACHE_MANAGER_H_

#include <string>
#include <unordered_map>

#include "cache_fs.h"
#include "cache_git_directory.h"

namespace falcon {

class Rule;
class Node;

class CacheManager {
 public:

  /** The cache policy dictates what is cached, and when. */
  enum class Policy {
    /* Nothing is cached. */
    CACHE_NOTHING,
    /* Everything is cached.
     * The cache will never be clear unless instructed by the user. */
    CACHE_EVERYTHING,
    /* Used if the project uses git. This will only cache when not in a detached
     * state. When building while on a given ref, all previous versions of the
     * target that were built under the same ref will be removed from the cache.
     * Basically, there's only one version of each target in the cache per git
     * branch. */
    CACHE_GIT_REFS
  };

  CacheManager(const std::string& workingDirectory,
               const std::string& falconDir);

  void setPolicy(Policy policy) { policy_ = policy; }
  Policy getPolicy() const { return policy_; }

  /**
   * Check the git repository for the current ref.
   * Must be called before each build.
   */
  void gitUpdateRef() { gitDirectory_.updateRef(); }

  /**
   * Called after a rule was built. Save all the outputs and the depfile
   * in cache.
   */
  void saveRule(Rule* rule);

  /**
   * Try to restore all the outputs of the given rule from the cache.
   * @param rule Rule to be restored.
   * @return true if the rule was successfully restored, ie all outputs were
   * found in cache.
   */
  bool restoreRule(Rule* rule);

  /**
   * Query the cache for information on the depfiles of the given rule.
   * Reload the depfile if found in cache.
   * @param rule Rule for which to check for implicit dependencies in the cache.
   * @return true if we were able to restore the depfile.
   */
  bool restoreDepfile(Rule* rule);

 private:
  /**
   * Save a node in cache.
   */
  bool saveNode(Node* node);

  Policy policy_;
  std::string workingDirectory_;
  CacheFS cacheFs_;
  CacheGitDirectory gitDirectory_;
};

} // namespace falcon

#endif // FALCON_CACHE_MANAGER_H_
