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

/**
 * Utility class that handles the spawning and monitoring of several
 * subprocesses.
 */
class PosixSubProcessManager {
 public:

  PosixSubProcessManager(IStreamConsumer *consumer);

  ~PosixSubProcessManager();

  /**
   * Spawn a new subprocess.
   * Add the stdout and stderr file descriptors of the given process to pollfds_
   * and map_ so that they can be monitored with ppoll.
   *
   * @param command Command to run.
   */
  void addProcess(const std::string& command,
                  const std::string& workingDirectory);

  /**
   * Use ppoll to monitor the file descriptors of the running processes and read
   * the output if a file descriptor is ready to be read.
   */
  void run();

  /**
   * Wait for the next command to complete.
   * This will call run until a command completes and return the command's exit
   * status.
   * @return Subprocess that completed.
   */
  PosixSubProcessPtr waitForNext();

  size_t nbRunning() const { return running_.size(); }

  /** Interrupt all the running processes by sending the SIGINT signal. */
  void interrupt();

 private:

  typedef std::vector<PosixSubProcessPtr> RunningProcesses;
  typedef std::queue<PosixSubProcessPtr>  FinishedProcesses;
  typedef std::vector<pollfd>             VectorFds;
  typedef std::list<pollfd>               ListFds;

  struct FdInfo {
    /* Position of subprocess in running_. */
    size_t runningPos;
    /* Position of fd in pollfds_. */
    ListFds::iterator itFd;
    /* Set to true if the fd is for stdout. Otherwise it is for stderr. */
    bool isStdout;
  };

  typedef std::unordered_map<int, FdInfo> Map;

  /**
   * Read a fd. If we encounter EOF, remove the fd from the list of fds to be
   * monitored with ppoll. If both the fds for stdout and stderr are closed for
   * this process, move it from running_ to finished_.
   * @param fd from which to read.
   */
  void readFd(int fd);

  unsigned int id_;
  IStreamConsumer* consumer_;

  /** List of processes currently running. */
  RunningProcesses running_;
  /** List of processes that have finished running. */
  FinishedProcesses finished_;
  /** list of file descriptors being monitored with ppoll. */
  ListFds pollfds_;
  /** When polling a read event from a fd, this map makes it possible to have
   * information about it, ie the corresponding subprocess, the position of the
   * fd in pollfds_, if it's stdout or stderr. See FdInfo. */
  Map map_;

  /* Protect concurrent access to running_. */
  std::mutex mutex_;
};

} // namespace falcon

#endif // FALCON_POSIX_SUBPROCESS_H_

