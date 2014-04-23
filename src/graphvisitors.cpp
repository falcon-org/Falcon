/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <graph.h>
#include <iostream>

namespace falcon {

void GraphMakefilePrinter::visit(Graph& g) {
  NodeSet const& nodeSet = g.getRoots();

  for (NodeSet::const_iterator it = nodeSet.cbegin();
       it != nodeSet.cend(); it++) {
    (*it)->accept(*this);
  }
}

void GraphMakefilePrinter::visit(Node& n) {
  Rule* child = n.getChild();
  if (child != nullptr) {
    child->accept(*this);
  }
}

void GraphMakefilePrinter::visit(Rule& r) {
  NodeArray const& inputs = r.getInputs();
  NodeArray const& outputs = r.getOutputs();

  for (NodeArray::const_iterator it = outputs.cbegin();
       it != outputs.cend(); it++) {
    std::cout << (*it)->getPath() << " ";
  }

  std::cout << ": ";

  for (NodeArray::const_iterator it = inputs.cbegin();
       it != inputs.cend(); it++) {
    std::cout << (*it)->getPath() << " ";
  }

  std::cout << std::endl;
  std::cout << "\t" << r.getCommand() << std::endl;

  for (NodeArray::const_iterator it = inputs.cbegin(); it != inputs.cend(); it++) {
    (*it)->accept(*this);
  }
}



void GraphGraphizPrinter::visit(Graph& g) {
  NodeMap const& nodeMap = g.getNodes();
  RuleArray const& rules = g.getRules();

  std::cout << "digraph Falcon {" << std::endl;
  std::cout << "rankdir=\"LR\"" << std::endl;
  std::cout << "node [fontsize=10, shape=box, height=0.25]" << std::endl;
  std::cout << "edge [fontsize=10]" << std::endl;

  /* print all the Nodes */
  for (auto it = nodeMap.cbegin(); it != nodeMap.cend(); it++) {
    it->second->accept(*this);
  }

  /* print all the rules */
  for (auto it = rules.cbegin(); it != rules.cend(); it++) {
    (*it)->accept(*this);
  }

  std::cout << "}" << std::endl;
}

void GraphGraphizPrinter::visit(Node& n) {
  std::cout << "\"" << &n << "\" [label=\""
            << n.getPath() << "\"]" << std::endl;
}

void GraphGraphizPrinter::visit(Rule& r) {
  NodeArray const& inputs = r.getInputs();
  NodeArray const& outputs = r.getOutputs();

  for (auto iit = inputs.cbegin(); iit != inputs.cend(); iit++) {
    for (auto oit = outputs.cbegin(); oit != outputs.cend(); oit++) {
      std::cout << "\"" << (*iit) << "\" ->"
                << "\"" << (*oit) << "\""<< std::endl;
    }

  }
}

}
