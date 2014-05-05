/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <openssl/sha.h>
#include <cassert>
#include <cstring>

#include "graph.h"
#include "graph_hash.h"

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
    /* The hash of the child rule should have been computed already. */
    assert(!child->getHash().empty());
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
    /* The hash of the inputs should have been computed already. */
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

}
