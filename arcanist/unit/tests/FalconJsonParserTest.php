<?php

class FalconJsonParserTest extends FalconUnitTestBase {
  public function getBinaryTest() {
    return "tests/jsonparser";
  }

  public function getDependencies() {
    return array(
      "src/json/tests/parser.cpp",
      "src/json/parser.cpp",
      "src/json/parser.h",
      "src/json/json.c",
      "src/json/json.h",
      "src/exceptions.h",
      "src/test.cpp",
      "src/test.h",
    );
  }
}
