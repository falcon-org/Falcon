/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>
#include <algorithm>

#include "graph.h"
#include "logging.h"
#include <sys/types.h>
#include <sys/stat.h>

namespace falcon {

/* ************************************************************************* */
/* Makefile printer                                                          */
/* ************************************************************************* */

GraphMakefilePrinter::GraphMakefilePrinter() : GraphVisitorPrinter(std::cout) {}
GraphMakefilePrinter::GraphMakefilePrinter(std::ostream& os) :
  GraphVisitorPrinter(os) {}

void GraphMakefilePrinter::visit(Graph& g) {
  auto rules = g.getRules();

  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    (*it)->accept(*this);
  }
}

void GraphMakefilePrinter::visit(Node& n) { /* nothing to do */ }

void GraphMakefilePrinter::visit(Rule& r) {
  auto inputs = r.getInputs();
  auto outputs = r.getOutputs();

  for (auto it = outputs.cbegin(); it != outputs.cend(); it++) {
    os_ << (*it)->getPath() << " ";
  }

  os_ << ": ";

  for (auto it = inputs.cbegin(); it != inputs.cend(); it++) {
    os_ << (*it)->getPath() << " ";
  }

  os_ << std::endl;
  os_ << "\t" << r.getCommand() << std::endl;
}

/* ************************************************************************* */
/* Graphiz printer                                                           */
/* ************************************************************************* */

GraphGraphizPrinter::GraphGraphizPrinter()
  : GraphVisitorPrinter(std::cout) {}
GraphGraphizPrinter::GraphGraphizPrinter(std::ostream& os) :
  GraphVisitorPrinter(os) {}

void GraphGraphizPrinter::visit(Graph& g) {
  NodeMap const& nodeMap = g.getNodes();
  RuleArray const& rules = g.getRules();

  os_ << "digraph Falcon {" << std::endl;
  os_ << "rankdir=\"LR\"" << std::endl;
  os_ << "edge [fontsize=10, arrowhead=vee]" << std::endl;

  /* print all the Nodes */
  for (auto it = nodeMap.cbegin(); it != nodeMap.cend(); it++) {
    it->second->accept(*this);
  }

  /* print all the rules */
  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    (*it)->accept(*this);
  }

  os_ << "}" << std::endl;
}

void GraphGraphizPrinter::visit(Node& n) {
  std::string color = (n.getState() == State::OUT_OF_DATE)
                    ? nodeColorOutOfDate_
                    : nodeColorUpToDate_;
  std::string fillColor = (n.getState() == State::OUT_OF_DATE)
                        ? nodeFillColorOutOfDate_
                        : nodeFillColorUpToDate_;

  os_ << "node [fontsize=10, shape=box, height=0.25, style=filled]" << std::endl;
  os_ << "\"" << &n << "\" [label=\"" << n.getPath()
                    << "\"  color=\"" << color
                    << "\"  fillcolor=\"" << fillColor
                    << "\" ]" << std::endl;
}

void GraphGraphizPrinter::visit(Rule& r) {
  NodeArray const& inputs = r.getInputs();
  NodeArray const& outputs = r.getOutputs();

  std::string color = (r.getState() == State::OUT_OF_DATE)
                    ? ruleColorOutOfDate_
                    : ruleColorUpToDate_;
  std::string fillColor = (r.getState() == State::OUT_OF_DATE)
                        ? nodeFillColorOutOfDate_
                        : nodeFillColorUpToDate_;

  os_ << "node [fontsize=10, shape=point, height=0.25, style=filled]"
    << std::endl;
  os_ << "\"" << &r << "\" [label=\"" << "rule"
                    << "\"  color=\"" << color
                    << "\"  fillcolor=\"" << fillColor
                    << "\" ]" << std::endl;

  for (auto iit = inputs.cbegin(); iit != inputs.cend(); iit++) {
      os_ << "\"" << (*iit) << "\" ->" << "\"" << &r
          << "\" [ color=\"" << color << "\"]" << std::endl;
  }

  for (auto oit = outputs.cbegin(); oit != outputs.cend(); oit++) {
      os_ << "\"" << &r << "\" ->" << "\"" << (*oit)
          << "\" [ color=\"" << color << "\"]" << std::endl;
  }
}

/* ************************************************************************* */
/* TimeStamp updater                                                         */
/* ************************************************************************* */

void GraphTimeStampUpdater::visit(Graph& g) {
  auto nodeMap = g.getNodes();
  auto rules = g.getRules();

  /* Update the timestamp of every node */
  for (auto it = nodeMap.cbegin(); it != nodeMap.cend(); it++) {
    it->second->accept(*this);
  }

  /* Update the state of every rules (also update the state of the nodes) */
  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    (*it)->accept(*this);
  }
}

/* Update the timestamp of a Node:
 * If it is an iNode (a linux file): stat will return the last modification
 *    time (st_mtime) : then we update the time stamp.
 * Else stat will return an error:
 *   if errno == (ENOENT or ENOTDIR):
 *      the node represents an action to perform (a rule will generate this
 *      node
 *   else stat was unable to return the stat for multiple reasons (concurent
 *        access or whatever: see "man 2 stat") I decided to not raised an
 *        exception: there is an error, but the rule's command may fixe it. */
void GraphTimeStampUpdater::visit(Node& n) {
  struct stat st;
  TimeStamp t = 0;

  if (stat(n.getPath().c_str(), &st) < 0) {
    if (errno != ENOENT && errno != ENOTDIR) {
      LOG(FATAL) << "stat(" << n.getPath()
                 << "): [" << errno << "] " << strerror(errno);
    }
  } else {
    t = st.st_mtime;
  }

  n.updateTimeStamp(t);

  /* if the timestamp changed since the last check (the previous time stamp is
   * smaller than the new one) or if the new timestamp is 0 (for example: the
   * file has been deleted), then mark the node dirty */
  if ((n.getPreviousTimeStamp() < n.getTimeStamp()) ||
      (n.getTimeStamp() == 0)) {
    n.markDirty();

    /* If this node also has a child, then mark it dirty too */
    Rule* child = n.getChild();
    if (child != nullptr) {
      child->markDirty();
    }
  } else {
    n.markUpToDate();
  }
}

void GraphTimeStampUpdater::visit(Rule& r) {
  /* Get the oldest output */
  auto outputs = r.getOutputs();
  auto itOldestOutput = std::min_element(outputs.cbegin(), outputs.cend(),
      [](Node const* o1, Node const* o2) {
        return o1->getTimeStamp() < o2->getTimeStamp();
      }
    );
  TimeStamp oldestOutputTimeStamp = (*itOldestOutput)->getTimeStamp();

  /* Get the newest input */
  auto inputs  = r.getInputs();
  auto itNewerInput = std::find_if(inputs.begin(), inputs.end(),
      [oldestOutputTimeStamp](Node const* inputNode) {
        return inputNode->getTimeStamp() > oldestOutputTimeStamp;
      }
    );

  /* if there is at least one input newer than the oldest ouput */
  if (itNewerInput != inputs.end()) {
    Node* input = *itNewerInput;

    Rule* child = input->getChild();
    if (child == nullptr) { /* in the case of the input is a leaf */
      input->setState(State::OUT_OF_DATE);
    }

    r.markDirty();
  } else {
    /* No newer input: the rule is up to date */
    r.markUpToDate();
  }

}

}
