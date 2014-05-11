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

static void printRuleMakefile(Rule const& r, std::ostream& os) {
  NodeArray const& inputs = r.getInputs();
  NodeArray const& outputs = r.getOutputs();

  for (auto it = outputs.cbegin(); it != outputs.cend(); it++) {
    os << (*it)->getPath() << " ";
  }

  os << ": ";

  for (auto it = inputs.cbegin(); it != inputs.cend(); it++) {
    os << (*it)->getPath() << " ";
  }

  os << std::endl;
  os << "\t" << r.getCommand() << std::endl;
}

void printGraphMakefile(Graph const& g, std::ostream& os) {
  RuleArray const& rules = g.getRules();

  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    printRuleMakefile(**it, os);
  }
}

/* ************************************************************************* */
/* Graphiz printer                                                           */
/* ************************************************************************* */

static void printNodeGraphiz(Node const& n, std::ostream& os) {
  std::string color = (n.getState() == State::OUT_OF_DATE)
                    ? "red"
                    : "black";
  std::string fillColor = "white";

  os << "node [fontsize=10, shape=box, height=0.25, style=filled]" << std::endl;
  os << "\"" << n.getHash() << "\" [label=\"" << n.getPath()
                   << "\"  color=\"" << color
                   << "\"  fillcolor=\"" << fillColor
                   << "\" ]" << std::endl;
}

static void printRuleGraphiz(Rule const& r, std::ostream& os) {
  NodeArray const& inputs = r.getInputs();
  NodeArray const& outputs = r.getOutputs();

  std::string color = (r.getState() == State::OUT_OF_DATE)
                    ? "red"
                    : "black";
  std::string fillColor = "white";

  if (r.isPhony()) {
    os << "node [fontsize=10, shape=circle, height=0.25, style=\"filled, dashed\""
      "]" << std::endl;
  } else {
    os << "node [fontsize=10, shape=circle, height=0.25, style=filled]"
      << std::endl;
  }

  os << "\"" << r.getHash() << "\" [label=\"" << r.getHash().substr(0, 5)
                   << "\"  color=\"" << color
                   << "\"  fillcolor=\"" << fillColor
                   << "\" ]" << std::endl;

  for (unsigned int i = 0; i < inputs.size(); ++i) {
    std::string shape;
    if (i < inputs.size() - r.getNumImplicitInputs()) {
      shape = "solid";
    } else {
      shape = "dashed";
    }

    std::string c = inputs[i]->getState() == State::OUT_OF_DATE
      ? "red" : "black";
    os << "\"" << inputs[i]->getHash() << "\" ->" << "\"" << r.getHash()
       << "\" [ color=\"" << c << "\", style=" << shape << " ]" << std::endl;
  }

  for (auto oit = outputs.cbegin(); oit != outputs.cend(); oit++) {
      os << "edge [fontsize=10, arrowhead=vee]" << std::endl;
      os << "\"" << r.getHash() << "\" ->" << "\"" << (*oit)->getHash()
         << "\" [ color=\"" << color << "\", style=solid]" << std::endl;
  }
}

void printGraphGraphiz(Graph const& g, std::ostream& os) {
  NodeMap const& nodeMap = g.getNodes();
  RuleArray const& rules = g.getRules();

  os << "digraph Falcon {" << std::endl;
  os << "rankdir=\"LR\"" << std::endl;
  os << "edge [fontsize=10, arrowhead=vee]" << std::endl;

  /* print all the Nodes */
  for (auto it = nodeMap.cbegin(); it != nodeMap.cend(); it++) {
    printNodeGraphiz(*(it->second), os);
  }

  /* print all the rules */
  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    printRuleGraphiz(*(*it), os);
  }

  os << "}" << std::endl;
}

}
