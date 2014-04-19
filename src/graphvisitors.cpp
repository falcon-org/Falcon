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

}
