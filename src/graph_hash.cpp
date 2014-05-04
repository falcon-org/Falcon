/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <openssl/sha.h>
#include <cassert>
#include <cstring>

#include "graph.h"

namespace falcon {

static std::string digestToString(unsigned char digest[SHA256_DIGEST_LENGTH]) {
  char mdString[SHA256_DIGEST_LENGTH*2 + 1];
  memset(mdString, 0, sizeof(mdString));
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
  }
  std::string tmp(mdString);

  return tmp;
}

/*****************************************************************************
 * Compute the Sha256Sum of the given node and set the new finger print. *****/
static void computeNodeSha256(Node& n) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  char buffer[256];
  auto child = n.getChild();
  std::ifstream ifs;
  SHA256_CTX ctx;
  SHA256_Init(&ctx);

  SHA256_Update(&ctx, n.getPath().c_str(), n.getPath().size());

  if (child == nullptr) {
    ifs.open(n.getPath(), std::ios::in | std::ios::binary);
    while (ifs) {
      ifs.read(buffer, sizeof(buffer));
      SHA256_Update(&ctx, buffer, ifs.gcount());
    }
    ifs.close();
  } else {
    SHA256_Update(&ctx, child->getHash().c_str(), child->getHash().size());
  }

  SHA256_Final(digest, &ctx);

  n.setHash(digestToString(digest));
}

static void computeRuleSha256(Rule& r) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  NodeArray& inputs = r.getInputs();

  SHA256_CTX ctx;
  SHA256_Init(&ctx);

  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    assert(!(*it)->getHash().empty());
    SHA256_Update(&ctx, (*it)->getHash().c_str(), (*it)->getHash().size());
  }

  SHA256_Update(&ctx, r.getCommand().c_str(), r.getCommand().size());

  SHA256_Final(digest, &ctx);

  r.setHash(digestToString(digest));
}

/*****************************************************************************
 * Methods to update the hash of a given Node or Rule.
 * These functions won't explore the graph ***********************************/
void updateNodeHash(Node& n) {
  computeNodeSha256(n);
}
void updateRuleHash(Rule& r) {
  computeRuleSha256(r);
}

/*****************************************************************************
 * Methods to initialize the Graph hashes
 * These functions will explore the graph recursively ***********************/

static void setRuleHash(Rule& r);

static void setNodeHash(Node& n) {
  auto child = n.getChild();

  if (child != nullptr && child->getHash().empty()) {
    setRuleHash(*child);
  }

  updateNodeHash(n);
}

static void setRuleHash(Rule& r) {
  NodeArray& inputs = r.getInputs();

  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    /* As different rules can have Nodes in common: no need to set the node
     * Hash. It hash been already set. */
    if ((*it)->getHash().empty()) {
      setNodeHash(**it);
    }
  }

  updateRuleHash(r);
}

void setGraphHash(Graph& g) {
  NodeSet& roots = g.getRoots();

  for (auto it = roots.begin(); it != roots.end(); it++) {
    /* This function is not designed to be called more than once.
     * If the root nodes have already a hash it's an error */
    assert((*it)->getHash().empty());
    setNodeHash(**it);
  }
}

}
