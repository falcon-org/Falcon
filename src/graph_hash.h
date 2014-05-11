/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_HASH_H_
#define FALCON_GRAPH_HASH_H_

namespace falcon { namespace hash {

/* Update the Node hash:
 * if it is a leaf, then compute the new hash. Else get the Child's hash.
 * It is expected that the child rule's hash was already computed. */
bool updateNodeHash(Node& n,
                    bool recomputeHash,
                    bool recomputeHashDeps);

/* Update the Rule hash: with the hash of it's inputs' hash.
 * It's expected that the hash of the inputs was already computed. */
void updateRuleHash(Rule& n,
                    bool recomputeHash,
                    bool recomputeHashDeps);

void recomputeRuleHash(Rule* rule,
                       WatchmanClient* watchmanClient,
                       Graph& graph,
                       CacheManager* cache,
                       bool recomputeHash,
                       bool recomputeHashDeps);

bool recomputeNodeHash(Node* node,
                       WatchmanClient* watchmanClient,
                       Graph& graph,
                       CacheManager* cache,
                       bool recomputeHash,
                       bool recomputeHashDeps);

} } // namespace falcon::hash

#endif // FALCON_GRAPH_HASH_H_
