<?php

class FalconPosixSubProcessTest extends FalconUnitTestBase {
  public function getBinaryTest() {
    return "tests/posix_subprocess";
  }

  public function getDependencies() {
    return array(
      "src/logging.cpp",
      "src/logging.h",
      "src/tests/posix_subprocess.cpp",
      "src/posix_subprocess.cpp",
      "src/posix_subprocess.h",
      "src/stream_consumer.cpp",
      "src/stream_consumer.h",
      "src/test.cpp",
      "src/test.h",
    );
  }
}
