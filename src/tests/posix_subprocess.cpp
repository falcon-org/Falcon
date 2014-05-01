/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "test.h"
#include "logging.h"
#include "posix_subprocess.h"
#include <iostream>

class FalconPosixSubprocessTest : public falcon::Test {
public:
  FalconPosixSubprocessTest(std::string const& cmd,
                            falcon::SubProcessExitStatus exitExpected,
                            std::string stdOut, std::string stdErr)
    : falcon::Test("Posix Process: " + cmd, "no error")
    , cmd_(cmd), psp_(nullptr), ssc_()
    , exitStatus_(exitExpected), stdOut_(stdOut), stdErr_(stdErr)
  {}

  void prepareTest() {
    falcon::initlogging(falcon::Log::Level::error);
    psp_ = new falcon::PosixSubProcess(cmd_, ".", 0, &ssc_);
  }
  void runTest() {
    psp_->start();
    psp_->waitFinished();
    if (exitStatus_ != psp_->status_) {
      setSuccess(false);
      setErrorMessage("wrong exit status");
      return;
    }

    if (!stdOut_.empty()) {
      if (psp_->readStdout() && ssc_.getCoutString().empty()) {
        setSuccess(false);
        setErrorMessage("Stdout expected");
        return;
      } else {
        if (ssc_.getCoutString() != stdOut_) {
          setSuccess(false);
          setErrorMessage("wrong stdout, expected(" + stdOut_ +
                          ") got(" + ssc_.getCoutString() + ")");
          return;
        }
      }
    }

    if (!stdErr_.empty()) {
      if (psp_->readStderr() && ssc_.getCerrString().empty()) {
        setSuccess(false);
        setErrorMessage("Stderr expected");
        return;
      } else {
        if (ssc_.getCerrString() != stdErr_) {
          setSuccess(false);
          setErrorMessage("wrong stderr, expected(" + stdErr_ +
                          ") got(" + ssc_.getCerrString() + ")");
          return;
        }
      }
    }

    setSuccess(true);
  }
  void closeTest() {
    delete psp_;
  }

private:
  std::string cmd_;
  falcon::PosixSubProcess* psp_;
  falcon::StringStreamConsumer ssc_;

  falcon::SubProcessExitStatus exitStatus_;
  std::string stdOut_;
  std::string stdErr_;
};


int main(int const argc, char const* const argv[]) {
  if (argc != 1 && argc != 2) {
    std::cerr << "usage: " << argv[0] << " [--json]" << std::endl;
    return 1;
  }

  falcon::TestSuite tests("Posix Subprocess test suite");

  tests.add(
    new FalconPosixSubprocessTest(
              "echo -n To STDOUT",
              falcon::SubProcessExitStatus::SUCCEEDED,
              "To STDOUT", "")
    );
  tests.add(
    new FalconPosixSubprocessTest(
              "echo -n To STDERR >&2",
              falcon::SubProcessExitStatus::SUCCEEDED,
              "", "To STDERR")
    );
  tests.add(
    new FalconPosixSubprocessTest(
              "echo -n To STDOUT >&1 ; echo -n To STDERR >&2",
              falcon::SubProcessExitStatus::SUCCEEDED,
              "To STDOUT", "To STDERR")
    );
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
