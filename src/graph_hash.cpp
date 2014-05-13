/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include <fstream>
#include <openssl/sha.h>
#include <cassert>
#include <cstring>

#include "graph.h"
#include "graph_hash.h"

#include "cache_manager.h"
#include "depfile.h"

#include "logging.h"

namespace falcon { namespace hash {

class Hasher {
 public:
  Hasher() {
    SHA256_Init(&ctx_);
  }

  Hasher& operator<<(const std::string& data) {
    SHA256_Update(&ctx_, data.c_str(), data.size());
    return *this;
  }

  std::string get() {
    SHA256_Final(digest_, &ctx_);
    char mdString[SHA256_DIGEST_LENGTH*2 + 1];
    memset(mdString, 0, sizeof(mdString));
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
      sprintf(&mdString[i*2], "%02x", (unsigned int)digest_[i]);
    }
    return mdString;
  }

 private:
  SHA256_CTX ctx_;
  unsigned char digest_[SHA256_DIGEST_LENGTH];
};

bool updateNodeHash(Node& n,
                    bool recomputeHash,
                    bool recomputeHashDeps) {
  assert(recomputeHash || recomputeHashDeps);
  auto child = n.getChild();
  bool changed = false;

  if (child == nullptr) {
    std::ifstream ifs;
    ifs.open(n.getPath(), std::ios::in | std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
    ifs.close();
    Hasher hasher;
    hasher << n.getPath() << data;
    std::string hash = hasher.get();
    if (recomputeHash) {
      changed |= n.getHash() != hash;
      n.setHash(hash);
    }
    if (recomputeHashDeps) {
      changed |= n.getHash() != hash;
      n.setHashDepfile(hash);
    }
  } else {
    if (recomputeHash) {
      assert(!child->getHash().empty());
      Hasher hasher;
      hasher << n.getPath() << child->getHash();
      std::string hash = hasher.get();
      changed |= n.getHash() != hash;
      n.setHash(hash);
    }
    if (recomputeHashDeps) {
      assert(!child->getHashDepfile().empty());
      Hasher hasher;
      hasher << n.getPath() << child->getHashDepfile();
      std::string hash = hasher.get();
      changed |= n.getHashDepfile() != hash;
      n.setHashDepfile(hash);
    }
  }

  return changed;
}

void updateRuleHash(Rule& r,
                    bool recomputeHash,
                    bool recomputeHashDeps) {
  NodeArray& inputs = r.getInputs();

  if (recomputeHash) {
    Hasher hasher;
    for (unsigned int i = 0; i < inputs.size(); i++) {
      assert(!inputs[i]->getHash().empty());
      hasher << inputs[i]->getHash();
    }
    r.setHash(hasher.get());
  }
  if (recomputeHashDeps) {
    Hasher hasher;
    for (unsigned int i = 0; i < inputs.size(); i++) {
      if (i < inputs.size() - r.getNumImplicitInputs()) {
        assert(!inputs[i]->getHashDepfile().empty());
        hasher << inputs[i]->getHashDepfile();
      }
    }
    r.setHashDepfile(hasher.get());
  }
}

void recomputeRuleHash(Rule* rule,
                       WatchmanClient* watchmanClient,
                       Graph& graph,
                       CacheManager* cache,
                       bool recomputeHash,
                       bool recomputeHashDeps) {
  std::string tmp = rule->getHashDepfile();

  updateRuleHash(*rule, true, true);

  /* If the deps hash changed, it means the implicit depfiles may have changed.
   * In that case, query the cache to see if we know the new implicit
   * dependencies. */
  if (rule->hasDepfile() && tmp != rule->getHashDepfile()) {
    if (cache->restoreDepfile(rule)) {
      Depfile::loadFromfile(rule->getDepfile(), rule, watchmanClient, graph,
                            true);
      /* The implicit dependencies may have changed, so recompute the normal
       * hash. We don't compute the deps hash again. */
      hash::updateRuleHash(*rule, true, false);
    }
  }

  auto& outputs = rule->getOutputs();
  for (auto it = outputs.begin(); it != outputs.end(); ++it) {
    recomputeNodeHash(*it, watchmanClient, graph, cache,
                      recomputeHash, recomputeHashDeps);
  }
}

bool recomputeNodeHash(Node* node,
                       WatchmanClient* watchmanClient,
                       Graph& graph,
                       CacheManager* cache,
                       bool recomputeHash,
                       bool recomputeHashDeps) {
  if (!updateNodeHash(*node, recomputeHash, recomputeHashDeps)) {
    /* The hash did not change. No need to recompute the hash of the parents. */
    LOG(INFO) << "hash of " << node->getPath() << "did not change";
    return false;
  }

  auto& parentRules = node->getParents();
  for (auto it = parentRules.begin(); it != parentRules.end(); ++it) {
    recomputeRuleHash(*it, watchmanClient, graph, cache,
                      recomputeHash, recomputeHashDeps);
  }

  return true;
}

} } // namespace falcon::hash
