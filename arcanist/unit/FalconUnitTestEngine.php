<?php

/**
 * Falcon unitary tests engine:
 *
 * For every file modified since the last commit, the engine will find the
 * corresponding tests to launch.
 *
 * To add a new test:
 *  -1- implement a CPP test suite (see src/test.h)
 *  -2- add the recipe to build it in CMakeLists.txt
 *      DO NOT FORGET to implement the --json option to your unitaty test
 *      program and to use the printJsonOutput from the TestSuite class.
 *  -3- implement a UnitTest: (see arcanist/unit/FalconUnitTestBase.php)
 *  -4- and you are done \o/ */

class FalconUnitTestEngine extends ArcanistBaseUnitTestEngine {

  protected $cmake;
  protected $projectRoot;
  protected $projectTest;
  protected $arrayOfTests;

  private  $listOfUnitTests;

  /* **********************************************************************
   * EDIT THE FOLLOWING FUNCTION TO ADD A TEST
   * ******************************************************************** */

  /**
   * Create the list of available test
   * To add a test, create a new FalconUnitTestBase and instanciate it here */
  private function buildListOfUnitTests() {
    $this->listOfUnitTests[] = new FalconJsonParserTest();
    $this->listOfUnitTests[] = new FalconExceptionTest();
    $this->listOfUnitTests[] = new FalconPosixSubProcessTest();
  }

  /* **********************************************************************
   * DO NOT EDIT THE FOLLOWING FUNCTIONS
   * ******************************************************************** */

  /**
   * Add an binary test in the array (avoid duplication) */
  private function addThisTestInArray($test) {
    if (empty($this->arrayOfTests) || !in_array($test, $this->arrayOfTests)) {
      $this->arrayOfTests[] = $test;
    }
  }

  /**
   * get the dependencies of a file:
   * if the file is linked to a test, then add this test to the arrayOfTests */
  private  function getArrayForTest($path) {
    foreach ($this->listOfUnitTests as $test) {
      if (in_array($path, $test->getDependencies())) {
        $this->addThisTestInArray($test->getBinaryTest());
      }
    }
  }

  /**
   * This test engine supports running all tests.
   */
  protected function supportsRunAllTests() {
    return true;
  }

  protected function loadEnvironment() {
    $this->projectRoot = $this->getWorkingCopy()->getProjectRoot();
    $this->projectTest = $this->projectRoot . "/UnitTests";

    // Determine build engine.
    if (Filesystem::binaryExists("cmake")) {
      $this->cmake = "cmake";
    } else {
      throw new Exception("Unable to find cmake PATH!");
    }
  }

  /**
   * Main entry point for the test engine.  Determines what assemblies to
   * build and test based on the files that have changed.
   *
   * @return array   Array of test results.
   */
  public function run() {
    $this->buildListOfUnitTests();
    $this->loadEnvironment();

    if ($this->getRunAllTests()) {
      $paths = id(new FileFinder($this->projectRoot))->find();
    } else {
      $paths = $this->getPaths();
    }

    foreach ($paths as $path) {
      $this->getArrayForTest($path);
    }

    return $this->runAllTests();
  }

  /**
   * Builds and run the requested test suites
   *
   * @return array   Array of test results.
   */
  public function runAllTests() {
    if (empty($this->arrayOfTests)) {
      return array();
    }

    $results = array();
    $results[] = $this->generateProject();
    if ($this->resultsContainFailures($results)) {
      return array_mergev($results);
    }
    $results[] = $this->buildProjects();
    if ($this->resultsContainFailures($results)) {
      return array_mergev($results);
    }

    $results[] = $this->testAssemblies();

    return array_mergev($results);
  }

  /**
   * Determine whether or not a current set of results contains any failures.
   * This is needed since we build the assemblies as part of the unit tests, but
   * we can't run any of the unit tests if the build fails.
   *
   * @param  array   Array of results to check.
   * @return bool    If there are any failures in the results.
   */
  private function resultsContainFailures(array $results) {
    $results = array_mergev($results);
    foreach ($results as $result) {
      if ($result->getResult() != ArcanistUnitTestResult::RESULT_PASS) {
        return true;
      }
    }
    return false;
  }

  /**
   * Generate the testing environment (cd tests && cmake ..):
   * If the 'tests' directory doesn't exist, we create it
   * @return array   Array of test results.
   */
  private function generateProject() {
    if (!is_dir(Filesystem::resolvePath($this->projectTest))) {
      mkdir($this->projectTest);
    }

    if (!is_dir(Filesystem::resolvePath($this->projectTest."/tests"))) {
      mkdir($this->projectTest."/tests");
    }

    $regenerate_start = microtime(true);

    /* use cmake to generate a Makefile */
    $regenerate_future = new ExecFuture(
      "%C %s",
      $this->cmake,
      $this->projectRoot);
    /* Set the working directory to avoid */
    $regenerate_future->setCWD(Filesystem::resolvePath($this->projectTest));
    $results = array();
    $result = new ArcanistUnitTestResult();
    $result->setName("(regenerate projects)");

    try {
      $regenerate_future->resolvex();
      $result->setResult(ArcanistUnitTestResult::RESULT_PASS);
    } catch(CommandException $exc) {
      if ($exc->getError() > 1) {
        throw $exc;
      }
      $result->setResult(ArcanistUnitTestResult::RESULT_FAIL);
      $result->setUserdata($exc->getStdout());
    }

    $result->setDuration(microtime(true) - $regenerate_start);
    $results[] = $result;
    return $results;
  }

  /**
   * Build the needed tests (not the entire project)
   *
   * @return array   Array of test results.
   */
  private function buildProjects() {
    $build_failed = false;
    $build_start = microtime(true);
    $results = array();

    foreach ($this->arrayOfTests as $test) {
      $future = new ExecFuture("make %s -j8", $test);
      $future->setCWD($this->projectTest);

      $result = new ArcanistUnitTestResult();
      $result->setName("Build: ". $test);

      try {
        $future->resolvex();
        $result->setResult(ArcanistUnitTestResult::RESULT_PASS);
      } catch(CommandException $exc) {
        if ($exc->getError() > 1) {
          throw $exc;
        }
        $result->setResult(ArcanistUnitTestResult::RESULT_FAIL);
        $result->setUserdata($exc->getStdout());
        $build_failed = true;
      }

      $result->setDuration(microtime(true) - $build_start);
      $results[] = $result;
    }

    return $results;
  }

  /**
   * Prepare and execute the tests */
  private function testAssemblies() {
    $allresults = array();

    // Build the futures for running the tests.
    $futures = array();
    foreach ($this->arrayOfTests as $test) {
      $future = new ExecFuture("%s --json", $test);
      $future->setCWD($this->projectTest);
      $futures[] = $future;
    }

    // Run all of the tests.
    foreach ($futures as $future) {
      $start_test = microtime(true);
      $json = $future->resolveJSON();
      $time = microtime(true) - $start_test;

      $results = $this->parseTestResult($json, $time);
      $allresults = array_merge($allresults, $results);
    }

    return $allresults;
  }

  private function parseTestResult($json, $time) {
    $name = $json['title'];
    $status = ArcanistUnitTestResult::RESULT_UNSOUND;
    $results = array();
    $fails = array();
    if ($json['success']) {
      $status = ArcanistUnitTestResult::RESULT_PASS;
    } else {
      $status = ArcanistUnitTestResult::RESULT_FAIL;
      foreach ($json['infos'] as $error) {
        $res = new ArcanistUnitTestResult();
        $res->setName($error['input']);
        $res->setResult($status);
        $res->setDuration(0);
        $res->setUserData("Error message: ".$error['error']);
        $fails[] = $res;
      }
    }

    $numberPassed = 0;
    $numberFailed = 0;
    if (!empty($json['passed'])) {
      $numberPassed = $json['passed'];
    }
    if (!empty($json['failed'])) {
      $numberFailed = $json['failed'];
    }
    $numberOfTests = $numberPassed + $numberFailed;

    $result = new ArcanistUnitTestResult();
    $result->setName($name . " (" . $numberPassed . "/" . $numberOfTests . ")");
    $result->setResult($status);
    $result->setDuration($time);
    switch ($numberFailed) {
    case 0:
      $result->setUserData("All test passed");
      break;
    case 1:
      $result->setUserData("1 test failed");
      break;
    default:
      $result->setUserData(($numberFailed) . " tests failed");
      break;
    }

    $results[] = $result;
    $results = array_merge($results, $fails);

    return $results;
  }

}
