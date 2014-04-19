/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <fstream>
#include <cstring>

#include <exceptions.h>
#include <iostream>
#include "json/json.h"
#include "graphparser.h"

namespace falcon {

GraphParser::GraphParser() { }

std::unique_ptr<Graph> GraphParser::getGraph() const {
  return std::unique_ptr<Graph>(new Graph(nodeSet_, nodeMap_));
}

void GraphParser::processFile(std::string const& filepath)
{
  JsonParser parser;

  { // Use a scope for input file stream reading
    std::ifstream ifs;
    unsigned int lineCounter = 0;

    ifs.open(filepath.c_str (), std::ifstream::in);
    do {
      /* TODO: change the way to read it from the json file */
      char lineBuffer[4096];
      memset(lineBuffer, 0, sizeof(lineBuffer));
      ifs.getline(lineBuffer, sizeof(lineBuffer) - 1);
      try {
        std::string theLine(lineBuffer);
        parser.parse(lineCounter++, theLine);
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

  // DEBUG: uncomment to show the rules DOM
  // std::cout << *rules;
}

void GraphParser::checkNode(JsonVal const* json, NodeArray& nodeArray) {
  for (std::deque<JsonVal*>::const_iterator it = json->_array.cbegin();
       it != json->_array.cend();
       it++) {
    JsonVal const* json_string = *it;

    if (json_string->_type != JSON_STRING) {
      THROW_ERROR(EINVAL, "invalid JSON entry: expect a STRING");
    }

    Node* node = this->nodeMap_[json_string->_data];
    if (!node) {
      node = new Node(json_string->_data);
      this->nodeMap_[json_string->_data] = node;
      this->nodeSet_.insert(node);
    }

    nodeArray.push_back(node);
  }
}

void GraphParser::processJson(JsonVal const* rules)
{
  for (std::deque<JsonVal*>::const_iterator it = rules->_array.cbegin();
       it != rules->_array.cend();
       it++) {
    NodeArray inputs;
    NodeArray outputs;

    JsonVal const* jsonRule = *it;

    if (jsonRule->_type != JSON_OBJECT_BEGIN) {
      continue;
    }

    JsonVal const* ruleInputs  = jsonRule->getObject("inputs");
    JsonVal const* ruleOutputs = jsonRule->getObject("outputs");
    JsonVal const* ruleCmd     = jsonRule->getObject("cmd");

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

    Rule* rule;
    if (ruleCmd) {
      if (ruleCmd->_type != JSON_STRING) {
        THROW_ERROR(EINVAL, "invalid entry: expect a STRING");
      }

      rule = new Rule(inputs, outputs, ruleCmd->_data);
    } else {
      rule = new Rule(inputs, outputs.front());
    }

    for (auto it = inputs.begin(); it != inputs.end(); it++) {
      (*it)->addParentRule(rule);
      this->nodeSet_.erase(*it);
    }
    for (auto it = outputs.begin(); it != outputs.end(); it++) {
      /* TODO: check that the rule has not already a child...
       * Warn: the assert will raise */
      (*it)->setChild(rule);
    }
  }
}
}
