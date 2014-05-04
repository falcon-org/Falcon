/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>

#include "graph.h"
#include "logging.h"

namespace falcon {

/* Update the timestamp of a Node. But do not change the state */
static void updateNodeTimestamp(Node& n) {
  struct stat st;
  Timestamp t = 0;

  if (stat(n.getPath().c_str(), &st) < 0) {
    if (errno != ENOENT && errno != ENOTDIR) {
      LOG(WARNING) << "Updating timestamp for Node '" << n.getPath()
                   << "' failed, this might affect the build system";
      DLOG(WARNING) << "stat(" << n.getPath() < ") [" << errno << "] " << strerror(errno);
    }
  } else {
    t = st.st_mtime;
  }

  n.setTimestamp(t);
}

/* Compare the oldest output to all of the input.
 * Mark as dirty every inputs which are newer than the oldest ouput. */
static void updateRuleTimestamp(Rule& r) {
  if (r.isDirty()) {
    /* If the rule is already dirty, then we won't need to analyse it:
     * save time and return now. */
    return;
  }

  /* Get the oldest output */
  auto outputs = r.getOutputs();
  auto itOldestOutput = std::min_element(outputs.cbegin(), outputs.cend(),
      [](Node const* o1, Node const* o2) {
        return o1->getTimestamp() < o2->getTimestamp();
      }
    );
  Timestamp oldestOutputTimestamp = (*itOldestOutput)->getTimestamp();

  /* mark dirty every input which are newer than the oldest output */
  auto inputs  = r.getInputs();
  for (auto it = inputs.begin(); it != inputs.end(); it++) {
    Node* input = *it;
    if (input->getTimestamp() > oldestOutputTimestamp) {
      input->markDirty();
    }
  }
}


void updateGraphTimestamp(Graph& g) {
  NodeMap& nodeMap = g.getNodes();
  RuleArray& rules = g.getRules();

  /* Update the timestamp of every node */
  for (auto it = nodeMap.begin(); it != nodeMap.end(); it++) {
    updateNodeTimestamp(*it->second);
  }

  /* Update the state of every rules */
  for (auto it = rules.begin(); it != rules.end(); it++) {
    updateRuleTimestamp(**it);
  }
}
}
