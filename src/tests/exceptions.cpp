/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "test.h"
#include "exceptions.h"

#include <iostream>

class FalconThrowExceptionTest : public falcon::Test {
public:
  FalconThrowExceptionTest()
    : falcon::Test("throw an exception", "no error")
  {}

  void prepareTest() { }
  void runTest() {
    try {
      throwException();
    } catch (falcon::Exception& e) {
      if (e.getCode() == EAGAIN) {
        setSuccess(true);
        setErrorMessage("succeful");
      } else {
        setSuccess(false);
        setErrorMessage("wrong error code");
      }
      return;
    }

    setSuccess(false);
    setErrorMessage("exception not thrown");
  }
  void closeTest() {}

private:
  void throwException() {
    THROW_ERROR_CODE(EAGAIN);
  }
};

class FalconForwardExceptionTest : public falcon::Test {
public:
  FalconForwardExceptionTest()
    : falcon::Test("forward an exception", "no error")
  {}

  void prepareTest() { }
  void runTest() {
    try {
      forwardException();
    } catch (falcon::Exception& e) {
      if (e.getCode() == EINTR) {
        setSuccess(true);
        setErrorMessage("succeful");
      } else {
        setSuccess(false);
        setErrorMessage("wrong error code");
      }
      return;
    }

    setSuccess(false);
    setErrorMessage("exception not forwarded");
  }
  void closeTest() {}

private:
  void forwardException() {
    try {
      throwException();
    } catch (falcon::Exception& e) {
      THROW_FORWARD_ERROR(e);
    }
  }
  void throwException() {
    THROW_ERROR_CODE(EINTR);
  }
};

int main(int const argc, char const* const argv[]) {
  if (argc != 1 && argc != 2) {
    std::cerr << "usage: " << argv[0] << " [--json]" << std::endl;
    return 1;
  }

  falcon::TestSuite tests("Exceptions test suite");

  tests.add(new FalconThrowExceptionTest());
  tests.add(new FalconForwardExceptionTest());
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
