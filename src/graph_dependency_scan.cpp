/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include <algorithm>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>

#include "cache_manager.h"
#include "depfile.h"
#include "graph_dependency_scan.h"
#include "graph.h"
#include "graph_hash.h"
#include "logging.h"

namespace falcon {

GraphDependencyScan::GraphDependencyScan(Graph& graph, CacheManager* cache)
    : graph_(graph)
    , cache_(cache) {}

void GraphDependencyScan::scan() {
  /* Update the timestamp of every node */
  NodeMap& nodeMap = graph_.getNodes();
  for (auto it = nodeMap.begin(); it != nodeMap.end(); it++) {
    /* Only stat the node if it is not the output of a phony rule. */
    if (!it->second->getChild() || !it->second->getChild()->isPhony()) {
      statNode(it->second);
    }
  }

  /* Now do a DFS on the whole graph to scan dependencies that need to be
   * rebuilt and recompute hashes. */
  auto& roots = graph_.getRoots();
  for (auto it = roots.begin(); it != roots.end(); ++it) {
    updateNode(*it);
  }
}

Node* GraphDependencyScan::getOldestOutput(Rule *r) {
  auto& outputs = r->getOutputs();
  assert(!r->getOutputs().empty());
  auto itOldestOutput = std::min_element(outputs.cbegin(), outputs.cend(),
      [](Node const* o1, Node const* o2) {
        return o1->getTimestamp() < o2->getTimestamp();
      }
    );
  return *itOldestOutput;
}

bool GraphDependencyScan::compareInputsWithOutputs(Rule *r) {
  bool atLeastOneDirty = false;

  /* Get the oldest output.  */
  Node* oldestOutput = getOldestOutput(r);
  assert(oldestOutput != nullptr);

  auto& inputs  = r->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    Node* input = *it;
    if (input->getTimestamp() > oldestOutput->getTimestamp()) {
      if (input->isSource()) {
        /* Only mark the input dirty if it is a source file. */
        input->setState(State::OUT_OF_DATE);
      }
      atLeastOneDirty = true;
    }
  }

  return atLeastOneDirty;
}

bool GraphDependencyScan::updateNode(Node* n) {
  bool dirty = false;

  if (!n->isSource()) {
    /* Traverse the child rule. */
    dirty = updateRule(n->getChild());
    /* If the child rule is dirty, this node is dirty. */
    if (dirty) {
      n->setState(State::OUT_OF_DATE);
    }
  }

  if (n->getHash().empty()) {
    hash::updateNodeHash(*n, true, true);
  }

  return dirty;
}

bool GraphDependencyScan::ruleLoadDepfile(Rule* r) {
  /* First, try to load the depfile. */
  auto res = Depfile::loadFromfile(r->getDepfile(), r, nullptr, graph_,
                                   false);
  if (res == Depfile::Res::SUCCESS) {
    return true;
  }

  /* We could not load the depfile, may be it is in cache. */
  if (!cache_->restoreDepfile(r)) {
    return false;
  }

  /* The depfile was retrieved from the cache. */
  res = Depfile::loadFromfile(r->getDepfile(), r, nullptr, graph_,
                              true);
  return res == Depfile::Res::SUCCESS;
}

/* Compare the oldest output to all of the input.
 * Mark as dirty every inputs which are newer than the oldest ouput. */
bool GraphDependencyScan::updateRule(Rule* r) {
  if (seen_.find(r) != seen_.end()) {
    return r->isDirty();
  }
  seen_.insert(r);

  bool isDirty = false;

  /* Traverse all the inputs. */
  auto& inputs  = r->getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    if (updateNode(*it)) {
      isDirty = true;
      if (!(*it)->isSource()) {
        /* If the input is not a source file, increase the counter of inputs
         * that are not ready for the rule. */
        r->markInputDirty();
      }
    }
  }

  hash::updateRuleHash(*r, true, true);

  if (r->hasDepfile()) {
    if (!ruleLoadDepfile(r)) {
      isDirty = true;
    } else {
      hash::updateRuleHash(*r, true, false);
    }
  }

  if (!r->isPhony() && compareInputsWithOutputs(r)) {
    isDirty = true;
  }

  if (isDirty) {
    r->setState(State::OUT_OF_DATE);
    return true;
  }

  return false;
}

void statNode(Node* n) {
  struct stat st;
  Timestamp t = 0;

  if (stat(n->getPath().c_str(), &st) < 0) {
    if (errno != ENOENT && errno != ENOTDIR) {
      LOG(WARNING) << "Updating timestamp for Node '" << n->getPath()
                   << "' failed, this might affect the build system";
      DLOG(WARNING) << "stat(" << n->getPath()
                    << ") [" << errno << "] " << strerror(errno);
    }
  } else {
    t = st.st_mtime;
  }

  n->setTimestamp(t);
}

} // namespace falcon
