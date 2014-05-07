/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_POSIX_SUBPROCESS_H_
#define FALCON_POSIX_SUBPROCESS_H_

#include <list>
#include <memory>
#include <mutex>
#include <poll.h>
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>

#include "exceptions.h"
#include "stream_consumer.h"

namespace falcon {

enum class SubProcessExitStatus { UNKNOWN, SUCCEEDED, INTERRUPTED, FAILED };
std::string toString(SubProcessExitStatus v);

/**
 * Utility class for spawning a posix sub process.
 */
class PosixSubProcess {
 public:
  explicit PosixSubProcess(const std::string& command,
                           const std::string& workingDirectory_,
                           unsigned int id,
                           IStreamConsumer* consumer);
  /** Start the process. */
  void start();

  /** Interrupt the process by sending the SIGINT signal. */
  void interrupt();

  /**
   * Entry point for the child process.
   * @param stdout File descriptor where the standard output should be
   *               written
   * @param stderr File descriptor where the error output should be written.
   */
  void childProcess(int stdout, int stderr);

  /* Return true if there is nothing more to read from stdout and stderr. */
  bool completed();

  /**
   * Try to read some data from fd, and write it to strBuf.
   * @param fd       Fd from which to read. Must be stdoutFd_ or stderrFd_.
   * @param isStdout Set to true if is stdout, false if stderr.
   * @return true if read EOF.
   */
  bool onFdReady(int& fd, bool isStdout);

  /**
   * Try to read some data from stdoutFd_.
   * @return true if read EOF.
   */
  bool readStdout() { return onFdReady(stdoutFd_, true); }

  /**
   * Try to read some data from stderrFd_.
   * @return true if read EOF.
   */
  bool readStderr() { return onFdReady(stderrFd_, false); }

  /**
   * Wait until a process has finished running.
   * This will block until the process actually completes.
   */
  void waitFinished();

  SubProcessExitStatus status() { return status_; }

  /**
   * Fill the given buffer with stdout.
   * @param buf Buffer to be filled.
   */
  std::string flushStdout();

  /**
   * Fill the given buffer with stdout.
   * @param buf Buffer to be filled.
   */
  std::string flushStderr();

  pid_t pid() const { return pid_; }

  unsigned int id_;

  std::string command_;
  std::string workingDirectory_;

  /* File descriptors from which to read the output and error streams.
   * Set to -1 if the file descriptor was closed. */
  int stdoutFd_;
  int stderrFd_;

  IStreamConsumer* consumer_;

  pid_t pid_;

  /* Exit status of the process. */
  SubProcessExitStatus status_;
};

typedef std::unique_ptr<PosixSubProcess> PosixSubProcessPtr;

} // namespace falcon

#endif // FALCON_POSIX_SUBPROCESS_H_
