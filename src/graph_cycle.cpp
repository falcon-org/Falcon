/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph.h"
#include "logging.h"
#include <cassert>
#include <sstream>
#include "exceptions.h"

namespace falcon {

typedef std::set<std::string> VisitedNodes;

void isCycle(Node const& n, VisitedNodes& stack) {
  if (stack.find(n.getPath()) != stack.cend()) {
    Exception e(__func__, __FILE__, __LINE__, EINVAL, "LOOP DETECTED IN GRAPH");
    std::string message = " +-> " + n.getPath();
    throw Exception(e.getErrorMessage(),
                    __func__, __FILE__, __LINE__,
                    e.getCode(), message.c_str());
  }

  Rule const* child = n.getChild();
  if (child == nullptr) {
    return;
  }

  auto pos = stack.insert(n.getPath());

  NodeArray const& inputs = child->getInputs();
  for (auto it = inputs.cbegin(); it != inputs.cend(); it++) {
    try {
      isCycle(**it, stack);
    } catch (Exception& e) {
      std::string message = " |   "+ n.getPath();
      throw Exception(e.getErrorMessage(), __func__, __FILE__, __LINE__,
                      e.getCode(), message.c_str());
    }
  }

  stack.erase(pos.first);
}

void checkGraphLoop(Graph const& g) {
  NodeSet const& roots = g.getRoots();

  for (auto it = roots.cbegin(); it != roots.cend(); it++) {
    VisitedNodes stack;

    isCycle(**it, stack);
  }
}


}
