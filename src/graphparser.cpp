/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <cassert>
#include <cstring>

#include "exceptions.h"
#include "depfile.h"
#include "graphparser.h"
#include "json/json.h"

namespace falcon {

GraphParser::GraphParser(std::string const& filepath)
  : graph_(new Graph())
  , graphFilePath_(filepath) {}

std::unique_ptr<Graph> GraphParser::getGraph() {
  return std::move(graph_);
}

void GraphParser::processFile()
{
  assert(graph_);
  JsonParser parser;

  {
    std::ifstream ifs;
    unsigned int lineCounter = 0;

    ifs.open(graphFilePath_.c_str (), std::ifstream::in);
    do {
      char buffer[4096];
      memset(buffer, 0, sizeof(buffer));
      ifs.read(buffer, sizeof(buffer) - 1);
      try {
        parser.parse(lineCounter++, buffer, ifs.gcount());
      } catch (Exception& e) {
        ifs.close();
        THROW_FORWARD_ERROR(e);
      }
    }  while(ifs.good ());
    ifs.close();
  }

  JsonVal* dom = parser.getDom();

  if (!dom) {
    THROW_ERROR(EINVAL, "No dom for this json file");
  }
  JsonVal const* rules = dom->getObject("rules");

  if (rules && rules->_type == JSON_ARRAY_BEGIN) {
    processJson(rules);
  } else {
    THROW_ERROR(EINVAL, "No rules in the given JSon File");
  }
}

void GraphParser::checkNode(JsonVal const* json, NodeArray& nodeArray) {
  for (std::deque<JsonVal*>::const_iterator it = json->_array.cbegin();
       it != json->_array.cend();
       ++it) {
    JsonVal const* json_string = *it;

    if (json_string->_type != JSON_STRING) {
      THROW_ERROR(EINVAL, "invalid JSON entry: expect a STRING");
    }

    Node* node = graph_->nodes_[json_string->_data];
    if (!node) {
      node = new Node(json_string->_data);
      graph_->nodes_[json_string->_data] = node;
      graph_->roots_.insert(node);
      graph_->sources_.insert(node);
    }

    nodeArray.push_back(node);
  }
}

void GraphParser::processJson(JsonVal const* rules)
{
  assert(graph_);

  for (std::deque<JsonVal*>::const_iterator it = rules->_array.cbegin();
       it != rules->_array.cend();
       ++it) {
    NodeArray inputs;
    NodeArray outputs;

    JsonVal const* jsonRule = *it;

    if (jsonRule->_type != JSON_OBJECT_BEGIN) {
      continue;
    }

    JsonVal const* ruleInputs  = jsonRule->getObject("inputs");
    JsonVal const* ruleOutputs = jsonRule->getObject("outputs");
    JsonVal const* ruleCmd     = jsonRule->getObject("cmd");
    JsonVal const* ruleDepfile = jsonRule->getObject("depfile");

    /* TODO: MANAGE ERROR ?
     * should I have to expect to have at least one input and one outputs ? */
    if (!ruleInputs && !ruleOutputs) {
      continue;
    }

    try {
      checkNode(ruleInputs, inputs);
      checkNode(ruleOutputs, outputs);
    } catch (Exception& e) {
      THROW_FORWARD_ERROR(e);
    }

    Rule* rule = new Rule(inputs, outputs);

    if (ruleCmd) {
      if (ruleCmd->_type != JSON_STRING) {
        THROW_ERROR(EINVAL, "Expecting STRING value for cmd field");
      }
      rule->setCommand(ruleCmd->_data);
    }

    if (ruleDepfile) {
      if (ruleDepfile->_type != JSON_STRING) {
        THROW_ERROR(EINVAL, "Expecting STRING value for depfile field");
      }
      rule->setDepfile(ruleDepfile->_data);
    }

    /* keep the rule in memory */
    graph_->rules_.push_back(rule);

    for (auto it = inputs.begin(); it != inputs.end(); it++) {
      (*it)->addParentRule(rule);
      rule->markInputReady();
      graph_->roots_.erase(*it);
    }
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      /* TODO: check that the rule does not already have a child...
       * Warn: the assert will raise */
      (*it)->setChild(rule);
      graph_->sources_.erase(*it);
    }

    /* Load the rule's implicit dependencies. */
    if (rule->hasDepfile()) {
      ruleLoadDepfile(rule);
    }
  }

  generateMandatoryNodes();
}

void GraphParser::ruleLoadDepfile(Rule* rule) {
  auto res = Depfile::loadFromfile(rule->getDepfile(), rule, nullptr, *graph_,
                                   false);
  if (res != Depfile::Res::SUCCESS) {
    /* Mark the rule as "missing depfile". Which means that it can't be marked
     * up to date until we resolve this. */
    rule->setMissingDepfile(true);
  }
}

void GraphParser::generateMandatoryNodes() {
  {
    /*  Register the graph file in order to manage it like every rule (register
     * to watchman, manage timestamp. */
    Node* nodeGraphFile = new Node(graphFilePath_);
    /*  Insert this node in the array of node */
    graph_->nodes_[graphFilePath_] = nodeGraphFile;
    graph_->roots_.insert(nodeGraphFile);
    graph_->sources_.insert(nodeGraphFile);
  }
}

}
