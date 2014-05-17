/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include "cache_manager.h"

#include "exceptions.h"
#include "fs.h"
#include "graph.h"
#include "logging.h"

namespace falcon {

CacheManager::CacheManager(const std::string& workingDirectory,
                           const std::string& falconDir)
    : workingDirectory_(workingDirectory)
    , cacheFs_(falconDir + "/cache")
    , gitDirectory_(workingDirectory, cacheFs_) {

  /* If we find a git repository, automatically use the CACHE_GIT_REFS
   * policy. */
  if (gitDirectory_.checkIsGitRepository()) {
    policy_ = Policy::CACHE_GIT_REFS;
  } else {
    policy_ = Policy::CACHE_EVERYTHING;
  }
}

bool CacheManager::saveNode(Node* node) {
  if (!cacheFs_.writeEntry(node->getHash(), node->getPath())) {
    LOG(ERROR) << "could not save " << node->getPath() << " in cache";
    return false;
  }

  if (policy_ == Policy::CACHE_GIT_REFS) {
    gitDirectory_.registerNode(node->getHash(), node);
  }

  return true;
}

void CacheManager::saveRule(Rule *rule) {
  if (rule->isPhony()) {
    return;
  }

  if (policy_ == Policy::CACHE_GIT_REFS && !gitDirectory_.isInRef()) {
    /* We are either in detached state, or there is no git repository. In either
     * case, do not store in cache. */
    return;
  }

  /* Save all the outputs. */
  auto& outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!saveNode(*it)) {
      LOG(ERROR) << "could not save " << (*it)->getPath() << " in cache";
    }
  }

  /* Save the depfile. */
  if (rule->hasDepfile()) {
    if (!cacheFs_.writeEntry(rule->getHashDepfile(), rule->getDepfile())) {
      LOG(ERROR) << "could not save " << rule->getDepfile() << " to "
        << rule->getHashDepfile() << std::endl;
    }
  }

  if (policy_ == Policy::CACHE_GIT_REFS) {
    gitDirectory_.registerRule(rule->getHashDepfile(), rule);
  }
}

bool CacheManager::restoreRule(Rule *rule) {
  if (rule->isPhony()) {
    return false;
  }

  /* Check we have all the outputs in cache. */
  auto& outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!cacheFs_.hasEntry((*it)->getHash())) {
      return false;
    }
    if (policy_ == Policy::CACHE_GIT_REFS) {
      gitDirectory_.registerNode((*it)->getHash(), *it);
    }
  }

  if (policy_ == Policy::CACHE_GIT_REFS) {
    gitDirectory_.registerRule(rule->getHashDepfile(), rule);
  }

  /* Retrieve all the outputs. */
  for (auto it = outputs.begin(); it != outputs.end(); it++) {
    if (!cacheFs_.readEntry((*it)->getHash(), (*it)->getPath())) {
      return false;
    }
  }

  return true;
}

bool CacheManager::restoreDepfile(Rule* rule) {
  return cacheFs_.readEntry(rule->getHashDepfile(), rule->getDepfile());
}

} // namespace falcon
