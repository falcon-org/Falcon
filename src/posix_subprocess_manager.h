/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_POSIX_SUBPROCESS_MANAGER_H_
#define FALCON_POSIX_SUBPROCESS_MANAGER_H_

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "posix_subprocess.h"
#include "graph.h"

namespace falcon {

/**
 * Utility class that handles the spawning and monitoring of several
 * subprocesses.
 */
class PosixSubProcessManager {
 public:

  struct BuiltRule {
    Rule* rule;
    SubProcessExitStatus status;
    unsigned int cmdId;
  };

  PosixSubProcessManager(IStreamConsumer *consumer);

  ~PosixSubProcessManager();

  /**
   * Spawn a new subprocess.
   * Add the stdout and stderr file descriptors of the given process to pollfds_
   * and map_ so that they can be monitored with ppoll.
   *
   * @param rule Rule to be built. Must not be a phony rule.
   * @return id of the command that was started.
   */
  unsigned int addProcess(Rule *rule, const std::string& workingDirectory);

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
  BuiltRule waitForNext();

  std::size_t nbRunning() const { return running_.size() + finished_.size(); }

  /** Interrupt all the running processes by sending the SIGINT signal. */
  void interrupt();

 private:

  typedef std::list<PosixSubProcessPtr>   RunningProcesses;
  typedef std::queue<PosixSubProcessPtr>  FinishedProcesses;
  typedef std::list<pollfd>               ListFds;

  struct FdInfo {
    /* Position of subprcess in running_. */
    RunningProcesses::iterator itRunning;
    /* Position of fd in pollfds_. */
    ListFds::iterator itFd;
    /* Set to true if the fd is for stdout. Otherwise it is for stderr. */
    bool isStdout;
  };

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
  std::unordered_map<int, FdInfo> map_;

  /* Protect concurrent access to running_. */
  std::mutex mutex_;

  /* This map makes it possible to retrieve the rule that corresponds to a
   * subprocess. */
  std::unordered_map<PosixSubProcess*, Rule*> mapToRule_;
};

} // namespace falcon

#endif // FALCON_POSIX_SUBPROCESS_MANAGER_H_
