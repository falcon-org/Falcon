/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <string>
#include <cassert>
#include <unordered_set>

#include "depfile.h"

#include "depfile_parser.h"
#include "exceptions.h"
#include "graph_hash.h"
#include "logging.h"
#include "watchman.h"

namespace falcon {

Node* Depfile::setRuleDependency(const std::string& dep, Rule* rule,
                                 WatchmanClient* watchmanClient, Graph& graph)
{
  Node* target;

  /* Find any existing node that matches the path. Create it if it does not
   * exist. */
  auto itFind = graph.getNodes().find(dep);
  bool isNewNode = itFind == graph.getNodes().end();
  if (isNewNode) {
    target = new Node(dep, false);
    hash::updateNodeHash(*target, true, true);
  } else {
    target = itFind->second;
    assert(target);
    /* TODO: isInput is not efficient because it has a linear complexity on the
     * number of inputs of the rule. Find a more efficient way to deal with
     * that. */
    if (rule->isInput(target)) {
      /* This node is already a dependency. */
      return target;
    }
  }

  DLOG(INFO) << target->getPath() << " is a new implicit dependency of "
    << rule->getOutputs()[0]->getPath();

  /* Set the target as a new input of the rule. */
  rule->addImplicitInput(target);
  /* Set the rule to be a parent of the target. */
  target->addParentRule(rule);

  if (isNewNode) {
    /* Notify the graph of the new node. */
    graph.addNode(target);
  }

  /* Update the counter of ready inputs for the rule. */
  if (target->isDirty() && !target->isSource()) {
    rule->markInputDirty();
  } else {
    rule->markInputReady();
  }

  /* Watch the target with watchman. */
  if (watchmanClient) {
    try {
      watchmanClient->watchNode(*target);
    } catch (Exception& e) {
      LOG(FATAL) << e.getErrorMessage();
    }
  }

  return target;
}

bool Depfile::load(std::string& buf, Rule *rule,
                   WatchmanClient* watchmanClient, Graph& graph,
                   bool logError) {

  /* Store the existing implcit deps in a set. */
  std::unordered_set<Node*> implicitDepsBefore;
  unsigned int numImplicitDeps = rule->getNumImplicitInputs();
  auto& inputs = rule->getInputs();
  for (unsigned int i = 0; i < numImplicitDeps; ++i) {
    Node *implicitDep = inputs[inputs.size() - i - 1];
    implicitDepsBefore.insert(implicitDep);
    if (!implicitDep->isDirty() || implicitDep->isSource()) {
      rule->markInputDirty();
    }
  }

  inputs.resize(inputs.size() - numImplicitDeps);
  rule->setNumImplicitInputs(0);

  DepfileParser depfile;
  string depfileErr;
  if (!depfile.Parse(&buf, &depfileErr)) {
    if (logError) {
      LOG(ERROR) << "Error parsing depfile: " << depfileErr;
    }
    return false;
  }

  /* Check that the output found in the depfile is the first output of the
   * rule. */
  std::string output = std::string(depfile.out_.str_, depfile.out_.len_);
  assert(!rule->getOutputs().empty());
  if (output != rule->getOutputs()[0]->getPath()) {
    if (logError) {
      LOG(ERROR) << "Invalid depfile. The output target does not match the first"
                    " output of the rule";
    }
    return false;
  }

  /* Add each input as a dependency of the rule. */
  for (auto it = depfile.ins_.begin(); it != depfile.ins_.end(); ++it) {
    std::string dep(const_cast<char*>(it->str_), it->len_);
    Node* node = setRuleDependency(dep, rule, watchmanClient, graph);
    implicitDepsBefore.erase(node);
  }

  /* All the nodes left in the set are not implicit deps anymore. */
  for (auto it = implicitDepsBefore.begin(); it != implicitDepsBefore.end();
      ++it) {
    Node* implicitDep = *it;
    implicitDep->removeParentRule(rule);
    if (implicitDep->getParents().empty() && !implicitDep->getChild()) {
      graph.getNodes().erase(implicitDep->getPath());
      graph.getRoots().erase(implicitDep);
      graph.getSources().erase(implicitDep);
      delete implicitDep;
    }
  }

  return true;
}

Depfile::Res Depfile::loadFromfile(const std::string& depfile, Rule *rule,
                                   WatchmanClient* watchmanClient,
                                   Graph& graph, bool logError) {
  std::ifstream ifs(depfile);
  if (!ifs.is_open()) {
    if (logError) {
      LOG(ERROR) << "Error, cannot open depfile " << depfile;
    }
    return Res::ERROR_CANNOT_OPEN;
  }

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      (std::istreambuf_iterator<char>()));
  if (content.empty()) {
    if (logError) {
      LOG(ERROR) << "Error, depfile " << depfile << " is empty";
    }
    return Res::ERROR_INVALID_FILE;
  }

  return load(content, rule, watchmanClient, graph, logError)
    ? Res::SUCCESS : Res::ERROR_INVALID_FILE;
}

} // namespace falcon
