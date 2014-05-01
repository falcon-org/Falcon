<?php

/**
 * Falcon Unit Test:
 *
 * In order to launch a test on the appropriate changes (arcanist will detect
 * every file changed since the last commit) you have to implement the
 * following interface and to add it in the "buildListOfUnitTest" function in
 * 'arcanist/unit/FalconUnitTestEngine.php'.
 *
 * For examples: 'aranist/unit/tests/*' */
abstract class FalconUnitTestBase {

  /**
   * Return the relative path to the binary file (or script) to execute the
   * test Suite.
   *
   * @return a string: the binary to execute to launch the test (should be the
   * same in CMakeLists.txt) */
  abstract function getBinaryTest();

  /**
   * @return an array of string: launch the test if one of the following files
   * changed */
  abstract  function getDependencies();
}
