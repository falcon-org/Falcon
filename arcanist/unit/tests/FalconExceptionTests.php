<?php

class FalconExceptionTest extends FalconUnitTestBase {
  public function getBinaryTest() {
    return "tests/exceptions";
  }

  public function getDependencies() {
    return array(
      "src/tests/exceptions.cpp",
      "src/exceptions.h",
      "src/test.cpp",
      "src/test.h",
    );
  }
}
