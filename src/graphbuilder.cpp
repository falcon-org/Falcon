/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <graphbuilder.h>
#include <fstream>
#include <cstring>

#include <exceptions.h>
#include <iostream>
#include <json/json.h>

namespace falcon {

GraphBuilder::GraphBuilder() { }

const std::vector<Rule*>& GraphBuilder::getResult() const {
  return this->result_;
}

void GraphBuilder::processFile(std::string const& filepath)
{
  JsonParser parser;

  {
    std::ifstream ifs;
    unsigned int lineCounter = 0;

    ifs.open (filepath.c_str (), std::ifstream::in);
    do {
      char lineBuffer[256];
      memset (lineBuffer, 0, sizeof (lineBuffer));
      ifs.getline (lineBuffer, sizeof (lineBuffer) - 1);
      try {
        std::string theLine (lineBuffer);
        parser.parse (lineCounter++, theLine);
      } catch (Exception& e) {
        ifs.close ();
        THROW_FORWARD_ERROR (e);
      }
    }  while (ifs.good ());
    ifs.close ();
  }

  JsonVal* dom = parser.getDom ();
  JsonVal const* rules = dom->getObject ("rules");

  if (rules && rules->_type == JSON_ARRAY_BEGIN) {
    this->processJson(rules);
  } else {
    THROW_ERROR(EINVAL, "No rules in the given JSon File");
  }

  // DEBUG: uncomment to show the rules DOM
  // std::cout << *rules;
}

void GraphBuilder::checkNode(JsonVal const* json, NodeArray& nodeArray) {
  for (std::deque<JsonVal*>::const_iterator it = json->_array.cbegin ();
       it != json->_array.cend ();
       it++) {
    JsonVal const* json_string = *it;

    if (json_string->_type != JSON_STRING) {
      THROW_ERROR(EINVAL, "invalid JSON entry: expect a STRING");
    }

    Node* node = this->nodeMap_[json_string->_data];
    if (!node) {
      node = new Node(json_string->_data);
      this->nodeMap_[json_string->_data] = node;
    }

    nodeArray.push_back(node);
  }
}

void GraphBuilder::processJson(JsonVal const* rules)
{
  for (std::deque<JsonVal*>::const_iterator it = rules->_array.cbegin();
       it != rules->_array.cend();
       it++) {
    NodeArray inputs;
    NodeArray outputs;

    JsonVal const* rule = *it;

    if (rule->_type != JSON_OBJECT_BEGIN) {
      continue;
    }

    JsonVal const* ruleInputs  = rule->getObject("inputs");
    JsonVal const* ruleOutputs = rule->getObject("outputs");
    JsonVal const* ruleCmd     = rule->getObject("cmd");

    /* TODO: MANAGE ERROR ?
     * should I have to expect to have at least one input and one outputs ? */
    if (!ruleInputs && !ruleOutputs) {
      continue;
    }

    try {
      this->checkNode(ruleInputs, inputs);
      this->checkNode(ruleOutputs, outputs);
    } catch (Exception& e) {
      THROW_FORWARD_ERROR(e);
    }

    if (ruleCmd) {
      if (ruleCmd->_type != JSON_STRING) {
        THROW_ERROR(EINVAL, "invalid entry: expect a STRING");
      }

      this->result_.push_back(new Rule(inputs, outputs, ruleCmd->_data));
    } else {
      this->result_.push_back(new Rule(inputs, outputs.front()));
    }
  }
}
}
