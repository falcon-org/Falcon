/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <string>

#include "depfile.h"

#include "depfile_parser.h"
#include "exceptions.h"
#include "logging.h"
#include "watchman.h"

namespace falcon {

void Depfile::setRuleDependency(const std::string& dep, Rule* rule,
                                WatchmanClient* watchmanClient, Graph& graph) {
  Node* target;

  /* Find any existing node that matches the path. Create it if it does not
   * exist. */
  auto itFind = graph.getNodes().find(dep);
  if (itFind == graph.getNodes().end()) {
    target = new Node(dep);
    graph.addNode(target);
  } else {
    target = itFind->second;
    assert(target);
    /* TODO: isInput is not efficient because it has a linear complexity on the
     * number of inputs of the rule. Find a more efficient way to deal with
     * that. */
    if (rule->isInput(target)) {
      /* This node is already a dependency. */
      return;
    }
  }

  LOG(debug) << target->getPath() << " is a new implicit dependency of "
    << rule->getOutputs()[0]->getPath();

  /* Set the target as a new input of the rule. */
  rule->addInput(target);
  /* Set the rule to be a parent of the target. */
  target->addParentRule(rule);

  /* Watch the target with watchman. */
  if (watchmanClient) {
    try {
      watchmanClient->watchNode(*target);
    } catch (Exception& e) {
      LOG(fatal) << e.getErrorMessage();
    }
  }
}

bool Depfile::load(std::string& buf, Rule *rule,
                   WatchmanClient* watchmanClient, Graph& graph) {
  DepfileParser depfile;
  string depfileErr;
  if (!depfile.Parse(&buf, &depfileErr)) {
    LOG(error) << "Error parsing depfile: " << depfileErr;
    return false;
  }

  /* Check that the output found in the depfile is the first output of the
   * rule. */
  std::string output = std::string(depfile.out_.str_, depfile.out_.len_);
  assert(!rule->getOutputs().empty());
  if (output != rule->getOutputs()[0]->getPath()) {
    LOG(error) << "Invalid depfile. The output target does not match the first"
                  " output of the rule";
    return false;
  }

  /* Add each input as a dependency of the rule. */
  for (auto it = depfile.ins_.begin(); it != depfile.ins_.end(); ++it) {
    std::string dep(const_cast<char*>(it->str_), it->len_);
    setRuleDependency(dep, rule, watchmanClient, graph);
  }

  return true;
}

Depfile::Res Depfile::loadFromfile(const std::string& depfile, Rule *rule,
                                   WatchmanClient* watchmanClient,
                                   Graph& graph) {
  std::ifstream ifs(depfile);
  if (!ifs.is_open()) {
    LOG(error) << "Error, cannot open depfile " << depfile;
    return Res::ERROR_CANNOT_OPEN;
  }

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      (std::istreambuf_iterator<char>()));
  if (content.empty()) {
    LOG(error) << "Error, depfile " << depfile << " is empty";
    return Res::ERROR_INVALID_FILE;
  }

  return load(content, rule, watchmanClient, graph)
    ? Res::SUCCESS : Res::ERROR_INVALID_FILE;
}

} // namespace falcon
