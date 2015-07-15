/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>

#include <git2.h>
#include <git2/repository.h>

#include "cache_git_directory.h"

#include "graph.h"
#include "logging.h"

namespace falcon {

CacheGitDirectory::CacheGitDirectory(const std::string& gitRepository,
                                     CacheFS cacheFs)
    : gitRepository_(gitRepository)
    , cacheFs_(cacheFs) { }

bool CacheGitDirectory::checkIsGitRepository() const {
  git_libgit2_init();
  git_repository *repo = NULL;
  bool found = !git_repository_open(&repo, gitRepository_.c_str());
  git_repository_free(repo);
  git_libgit2_shutdown();
  return found;
}

void CacheGitDirectory::updateRef() {
  currentGitRef_ = "";
  git_repository *repo = NULL;
  git_reference *head = NULL;

  git_libgit2_init();
  int r = git_repository_open(&repo, gitRepository_.c_str());
  if (r != 0) {
    LOG(INFO) << "Cannot open git repository " << gitRepository_;
    goto cleanup;
  }

  if (git_repository_head_detached(repo)) {
    LOG(INFO) << "We are in detached state";
    goto cleanup;
  }

  r = git_repository_head(&head, repo);
  if (r != 0) {
    LOG(INFO) << "Cannot get current ref head";
    goto cleanup;
  }

  currentGitRef_ = git_reference_name(head);
  LOG(INFO) << "found ref: " << currentGitRef_;

cleanup:
  git_repository_free(repo);
  git_libgit2_shutdown();
}

bool CacheGitDirectory::isInRef() const {
  return !currentGitRef_.empty();
}

void CacheGitDirectory::registerNode(const std::string& hash, Node* node) {
  auto itRefMap = gitNodeMap_.find(node);
  if (itRefMap == gitNodeMap_.end()) {
    auto itInserted = gitNodeMap_.insert(std::make_pair(node, RefMap()));
    itRefMap = itInserted.first;
  }
  registerEntryInRefMap(hash, itRefMap->second);
}

void CacheGitDirectory::registerRule(const std::string& hash, Rule* rule) {
  auto itRefMap = gitRuleMap_.find(rule);
  if (itRefMap == gitRuleMap_.end()) {
    auto itInserted = gitRuleMap_.insert(std::make_pair(rule, RefMap()));
    itRefMap = itInserted.first;
  }
  registerEntryInRefMap(hash, itRefMap->second);
}

void CacheGitDirectory::registerEntryInRefMap(const std::string& hash,
                                              RefMap& refMap) {
  assert(isInRef());

  /* Find the cache entry that corresponds to this hash.
   * Create a new entry if not found. */
  auto itFind = gitHashMap_.find(hash);
  GitCacheEntry* entry;
  if (itFind == gitHashMap_.end()) {
    entry = new GitCacheEntry();
    entry->hash = hash;
    gitHashMap_[hash] = entry;
  } else {
    entry = itFind->second;
  }

  /* Look in the refMap for any CacheEntry that was already linked to the
   * current git ref. */
  auto itPrevEntry = refMap.find(currentGitRef_);
  if (itPrevEntry != refMap.end()) {
    GitCacheEntry* prevEntry = itPrevEntry->second;
    assert(prevEntry->numGitRefs > 0);
    prevEntry->numGitRefs--;
    if (prevEntry->numGitRefs == 0 && prevEntry != entry) {
      DLOG(INFO) << "deleting " << prevEntry->hash;
      cacheFs_.delEntry(prevEntry->hash);
      gitHashMap_.erase(prevEntry->hash);
      delete prevEntry;
    }
  }

  refMap[currentGitRef_] = entry;
  entry->numGitRefs++;
}

} // namespace falcon
