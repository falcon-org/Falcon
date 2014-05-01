/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "test.h"

#include "json/parser.h"
#include <iostream>

#include "exceptions.h"

std::string typeString(int type) {
  std::string s;
  switch(type) {
  case JSON_OBJECT_BEGIN:
    s = "JSON_OBJECT_BEGIN";
    break;
  case JSON_OBJECT_END:
    s = "JSON_OBJECT_END";
    break;
  case JSON_ARRAY_BEGIN:
    s = "JSON_ARRAY_BEGIN";
    break;
  case JSON_ARRAY_END:
    s = "JSON_ARRAY_END";
    break;
  case JSON_FALSE:
    s = "JSON_FALSE";
    break;
  case JSON_TRUE:
    s = "JSON_FALSE";
    break;
  case JSON_NULL:
    s = "JSON_NULL";
    break;
  case JSON_INT:
    s = "JSON_INT";
    break;
  case JSON_STRING:
    s = "JSON_STRING";
    break;
  case JSON_FLOAT:
    s = "JSON_FLOAT";
    break;
  default:
    char buff[sizeof(int)*8+1];
    memset(buff, 0, sizeof(buff));
    int ret = snprintf(buff, sizeof(buff), "0x%03x", type);
    s = "Unknown type: " + std::string(buff, ret);
    break;
  }
  return s;
}

class JsonParserTest : public falcon::Test {
public:
  JsonParserTest(std::string input,
                 int errorCode,
                 std::string key, std::string data, int type)
    : Test(input, "")
    , parser_(), input_(input)
    , errorCodeExpected_(errorCode)
    , key_(key), data_(data), type_(type)
  {}
  ~JsonParserTest();

  void prepareTest() { }
  void closeTest() { }

  void runTest() {
    try {
      parser_.parse(0, input_);
    } catch (falcon::Exception& e) {
      setErrorMessage("parsing error: " + e.getErrorMessage());
      setSuccess(e.getCode() == errorCodeExpected_);
      return;
    }

    falcon::JsonVal* dom = parser_.getDom();
    if (!dom) {
      if (key_ == "") {
        if (errorCodeExpected_ != 0) {
          setErrorMessage("wrong error code");
        }
        setSuccess(errorCodeExpected_ == 0);
      } else {
        setErrorMessage("parsing error: not a valide JSon");
        setSuccess(errorCodeExpected_ == EINVAL);
      }
      return;
    }

    falcon::JsonVal* obj = (falcon::JsonVal*)dom->getObject(key_);
    if (!obj) {
      if (key_ == "") {
        if (errorCodeExpected_ != 0) {
          setErrorMessage("wrong error code");
        }
        setSuccess(errorCodeExpected_ == 0);
      } else {
        setErrorMessage("object not found: " + key_);
        setSuccess(false);
      }
      return;
    }

    if (obj->_type != type_) {
      setErrorMessage("object type error: expected(" + typeString(type_)
                      + ") found(" + typeString(obj->_type) + ")");
      setSuccess(false);
      return;
    }

    if (obj->_data == data_) {
      setSuccess(errorCodeExpected_ == 0);
    } else {
      setErrorMessage("object do not have the same value: expected("
                      + data_ + ") found(" + obj->_data + ")");
      setSuccess(false);
    }
  }

private:
  falcon::JsonParser parser_;
  std::string input_;
  int errorCodeExpected_;

  std::string key_;
  std::string data_;
  int type_;
};


char const longJsonInput[] = "{ \n"
"  \"Authors\": [ \"Adrien CONRATH\", \"Nicolas DI PRIMA\" ],\n"
"  \"title\": \"Falcon\",\n"
"  \"type\": \"Build System\",\n"
"  \"license\" : \"BSD-3\",\n"
"  \"description\": [\n"
"    \"This an implementation of a perfect build system.\",\n"
"    \"Falcon provides features which make this project completely awesome\",\n"
"    \" and definitely more scalable and maleable than lot of other build\",\n"
"    \" systems.\"\n"
"  ],\n"
"  \"features\" : [\n"
"    { \"modules\" : [\n"
"        { \"name\" : \"dot\",  \"comment\" : \"print the Graph in a Dot/Graphiz format\"},\n"
"        { \"name\" : \"make\", \"comment\" : \"print the Graph in a Makefile format\"}\n"
"      ]\n"
"    },\n"
"    \"daemonize\",\n"
"    \"sequential build\",\n"
"    { \"source monitoring\" : [\n"
"        { \"type\" : \"stat\",     \"comment\" : \"compare timestamp\"},\n"
"        { \"type\" : \"watchman\", \"comment\" : \"use facebook/watchman\"}\n"
"      ]\n"
"    }\n"
"  ],\n"
"  \"date\" : \"2014\"\n"
"}\n";

int main(int const argc, char const* const argv[]) {
  if (argc != 1 && argc != 2) {
    std::cerr << "usage: " << argv[0] << " [--json]" << std::endl;
    return 1;
  }

  falcon::TestSuite tests("Json Parser test suite");

#define AddTest(input, code, key, data, type) \
  tests.add(new JsonParserTest(input, code, key, data, type))

  AddTest("{\"key\" : \"nicolas\"}", 0, "key", "nicolas", JSON_STRING);
  AddTest("{ \"key1\" : false, \"key2\" : 1222, \"key3\" : \"data\"}"
         , 0, "key1", "", JSON_FALSE);
  AddTest("{ \"key1\" : false, \"key2\" : 1222, \"key3\" : \"data\"}"
         , 0, "key2", "1222", JSON_INT);
  AddTest("{ \"key1\" : false, \"key2\" : 1222, \"key3\" : \"data\"}"
         , 0, "key3", "data", JSON_STRING);
  AddTest("{ \"array\" : [] }", 0, "array", "", JSON_ARRAY_BEGIN);
  AddTest("{ \"object\" : {} }", 0, "object", "", JSON_OBJECT_BEGIN);
  AddTest("[ \"array\", 10, true ]", 0, "", "", 0);

  AddTest(longJsonInput, 0, "date", "2014", JSON_STRING);

  AddTest("{ ", 0, "", "", 0);
  AddTest("{ = ", EINVAL, "", "", 0);
  AddTest("{ \"key\" : unknow}", EINVAL, "", "", 0);

  tests.run();


  if (argc == 2) {
    std::string option(argv[1]);
    if (option.compare("--json") == 0) {
      tests.printJsonOutput(std::cout);
    } else {
      std::cerr << "usage: " << argv[0] << " [--json]" << std::endl;
      return 1;
    }
  } else {
    tests.printStandardOutput(std::cout);
  }

  return 0;
}


