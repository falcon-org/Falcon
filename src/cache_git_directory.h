/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_CACHE_GIT_DIRECTORY_H_
# define FALCON_CACHE_GIT_DIRECTORY_H_

#include <string>
#include <unordered_map>

#include "cache_fs.h"

namespace falcon {

class Node;
class Rule;

/**
 * Directory of cache entries for a git repository.
 *
 * This class is used when the cache manager uses the CACHE_GIT_REFS policy.
 * It keeps track of cache entries associated to nodes and make sure that for a
 * given node there is only one cache entry per git reference.
 *
 * When a cache entry is added for a node, this class checks if there was a
 * previous entry for the same node and the same ref. If it is the case, the
 * previous entry is removed from the cache.
 */
class CacheGitDirectory {
 public:
  CacheGitDirectory(const std::string& gitRepository, CacheFS cacheFs);

  /** Return true if there is a git repository. */
  bool checkIsGitRepository() const;

  /** Inspect the git repository to find out which ref it is currently in. */
  void updateRef();

  /** Return true if the git repository is currently known as being in a
   * reference instead of being in a detached state. */
  bool isInRef() const;

  /** Notify that an entry has been saved in cache for the given node. */
  void registerNode(const std::string& hash, Node* node);

  /** Notify that an entry has been saved in cache for the given rule. */
  void registerRule(const std::string& hash, Rule* rule);

 private:
  std::string gitRepository_;

  /** Current git reference. Empty string if we are in a detached state or there
   * is no git repository. */
  std::string currentGitRef_;

  struct GitCacheEntry {
    std::string hash;
     /* Number of git refs that need this entry.
     * When this number reaches 0, the cache entry can be removed. */
    unsigned int numGitRefs;
  };

  /* For a given node, map a git ref to a cache entry.
   * Each node/rule has such a map so that we can track, for each node/rule, the
   * current cache entry associated with a git reference. */
  typedef std::unordered_map<std::string, GitCacheEntry*> RefMap;
  std::unordered_map<Node*, RefMap> gitNodeMap_;
  std::unordered_map<Rule*, RefMap> gitRuleMap_;

  /* Global map of cache entries, key'ed by hash. */
  std::unordered_map<std::string, GitCacheEntry*> gitHashMap_;

  void registerEntryInRefMap(const std::string& hash, RefMap& refMap);

  CacheFS cacheFs_;
};

} // namespace falcon

#endif // FALCON_CACHE_GIT_DIRECTORY_H_
