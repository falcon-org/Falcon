/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>

#include "graph_dependency_scan.h"
#include "graph.h"
#include "graph_hash.h"
#include "logging.h"

namespace falcon {

GraphDependencyScan::GraphDependencyScan(Graph& graph)
    : graph_(graph) {}

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
  NodeSet roots = graph_.getRoots();
  for (auto it = roots.begin(); it != roots.end(); ++it) {
    updateNode(*it);
  }
}

void GraphDependencyScan::statNode(Node* n) {
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

Node* GraphDependencyScan::getOldestOutput(Rule *r) {
  auto outputs = r->getOutputs();
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

  auto inputs  = r->getInputs();
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

  updateNodeHash(*n);

  return dirty;
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
  auto inputs  = r->getInputs();
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

  updateRuleHash(*r);

  /* If any of the inputs are dirty, the rule is dirty and we don't need to
   * compare the timestamps. */
  if (isDirty) {
    r->setState(State::OUT_OF_DATE);
    return true;
  }

  if (!r->isPhony()) {
    /* If we get here, none of the inputs are known dirty so far. However, they
     * might be if one of the outputs is older than of the inputs. */
    isDirty |= compareInputsWithOutputs(r);

    /* The rule is dirty if we could not find its depfile. */
    isDirty |= r->missingDepfile();
  }

  if (isDirty) {
    r->setState(State::OUT_OF_DATE);
    return true;
  }
  return false;
}

} // namespace falcon
