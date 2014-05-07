/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_CONSISTENCY_CHECKER_H_
#define FALCON_GRAPH_CONSISTENCY_CHECKER_H_

#include <mutex>

#include "graph.h"

#include "logging.h"

namespace falcon {

/**
 * A utility class for checkin the consistency of a graph.
 * This will browse the graph and check that every invariant on the graph is
 * true. If an error is encountered, it is logged.
 */
class GraphConsistencyChecker {
public:
  GraphConsistencyChecker(Graph* g);

  /* Perform a Depth-First traversal of the graph, starting from the root nodes.
   * Keep track of the number of roots, sources, rules seen and check that it
   * corresponds to the sets maintained by the graph. */
  virtual void check();

private:

  /**
   * Useful class for logging something only if a given condition is not met.
   * the trick is to create a temporary object like this:
   *
   * Checker(my_condition) << "this text is logged if the condition is not met";
   */
  struct Checker : public std::ostringstream {
    bool cond_;
    Checker(bool cond, GraphConsistencyChecker* c) : cond_(cond) {
      if (!cond) { c->isConsistent_ = false; }
    }
    ~Checker() {
      /* On destruction, log everything if the condition was not met. */
      if (!cond_) { LOG(ERROR) << std::ostringstream::str(); }
    }
  };

  virtual void checkNode(Node *node);
  virtual void checkRule(Rule *rule);

  Graph* graph_;
  std::size_t nbRootsSeen_;
  std::size_t nbSourcesSeen_;

  NodeSet nodesSeen_;
  RuleSet rulesSeen_;

  bool isConsistent_;

  friend struct Checker;
};

} // namespace falcon

#if defined (DEBUG)
# define FALCON_CHECK_GRAPH_CONSISTENCY(__graph__, __mutex__) \
{                                                             \
  std::lock_guard<std::mutex> lock(__mutex__);                \
  falcon::GraphConsistencyChecker checker(__graph__);         \
  checker.check();                                            \
}
#else
# define FALCON_CHECK_GRAPH_CONSISTENCY(__graph__, __mutex__)
#endif

#endif // FALCON_GRAPH_CONSISTENCY_CHECKER_H_
