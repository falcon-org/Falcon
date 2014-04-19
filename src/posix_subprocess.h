/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_POSIX_SUBPROCESS_H_
#define FALCON_POSIX_SUBPROCESS_H_

#include <string>

namespace falcon {

enum class SubProcessExitStatus { SUCCEEDED, INTERRUPTED, FAILED };

class PosixSubProcess {
 public:
  PosixSubProcess(const std::string& command);

  /** run the process. Block until it exits. */
  SubProcessExitStatus start();

 private:
  std::string command_;
};

} // namespace falcon

#endif // FALCON_POSIX_SUBPROCESS_H_

