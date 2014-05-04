/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "graph.h"
#include "logging.h"

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

}
