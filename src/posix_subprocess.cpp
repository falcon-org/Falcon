/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include <cassert>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "exceptions.h"
#include "posix_subprocess.h"
#include "logging.h"

namespace falcon {

PosixSubProcess::PosixSubProcess(const std::string& command,
                                 const std::string& workingDirectory)
  : command_(command)
  , workingDirectory_(workingDirectory)
  , stdoutFd_(-1), stderrFd_(-1)
  , pid_(-1), status_(SubProcessExitStatus::UNKNOWN) { }

void PosixSubProcess::start() {
  /* Create pipe for stdout redirection. */
  int stdout_pipe[2];
  if (pipe(stdout_pipe) < 0) {
    THROW_ERROR(errno, "Failed to open pipe");
  }
  stdoutFd_ = stdout_pipe[0];

  /* Create pipe for stderr redirection. */
  int stderr_pipe[2];
  if (pipe(stderr_pipe) < 0) {
    THROW_ERROR(errno, "Failed to open pipe");
  }
  stderrFd_ = stderr_pipe[0];

  pid_ = fork();
  if (pid_ < 0) {
    THROW_ERROR(errno, "Failed to fork");
  }

  if (pid_ == 0) {
    /* Child process. */
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    childProcess(stdout_pipe[1], stderr_pipe[1]);
    assert(false);
  }

  /* Parent process. */
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);
}

void PosixSubProcess::childProcess(int stdout, int stderr) {
  do {
    /* Use /dev/null for stdin. */
    int devnull = open("/dev/null", O_RDONLY);
    if (devnull < 0) { break; }

    /* Change stdin to be /dev/null. */
    if (dup2(devnull, 0) < 0) { break; }
    close(devnull);
    /* Change stdout. */
    if (dup2(stdout, 1) < 0) { break; }
    close(stdout);
    /* Change stderr. */
    if (dup2(stderr, 2) < 0) { break; }
    close(stderr);

    /* Since we have forked, we can change the directory before executing the
     * command line */
    if (chdir (workingDirectory_.c_str()) != 0) {

      LOG(fatal) << "unable to set the working directory before executing the command";
      THROW_ERROR_CODE(errno);
    }

    /* Run the command. */
    execl("/bin/sh", "/bin/sh", "-c", command_.c_str(), (char *) NULL);
  } while(false);

  /* If we get here, something failed. TODO: display error message to user. */
  _exit(1);
}

bool PosixSubProcess::completed() {
  return stdoutFd_ == -1 && stderrFd_ == -1;
}

bool PosixSubProcess::onFdReady(int& fd, std::string& strBuf) {
  assert(fd != -1);
  char buf[4 << 10];
  ssize_t len = read(fd, buf, sizeof(buf));
  if (len > 0) {
    strBuf.append(buf, len);
  } else if (len < 0) {
    THROW_ERROR(errno, "Read error");
  } else /* EOF */ {
    close(fd);
    fd = -1;
    return true;
  }
  return false;
}

void PosixSubProcess::waitFinished() {
  int status;
  if (waitpid(pid_, &status, 0) < 0) {
    THROW_ERROR(errno, "waitpid failed");
  }

  if (WIFEXITED(status)) {
    int r = WEXITSTATUS(status);
    if (r == 0) {
      status_ = SubProcessExitStatus::SUCCEEDED;
      return;
    }
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT) {
      status_ = SubProcessExitStatus::INTERRUPTED;
      return;
    }
  }
  status_ = SubProcessExitStatus::FAILED;
}

std::string PosixSubProcess::flushStdout() {
  return std::move(stdoutBuf_);
}

std::string PosixSubProcess::flushStderr() {
  return std::move(stderrBuf_);
}

PosixSubProcessManager::~PosixSubProcessManager() {
  /* The user should wait for all the processes to complete before
   * destroying this object. */
  assert(nbRunning() == 0);
}

void PosixSubProcessManager::addProcess(const std::string& command,
                                        const std::string& workingDirectory) {
  PosixSubProcessPtr proc(new PosixSubProcess(command, workingDirectory));
  proc->start();
  int stdout = proc->stdoutFd_;
  int stderr = proc->stderrFd_;

  running_.push_back(std::move(proc));
  size_t runningPos = running_.size() - 1;

  short events = POLLIN | POLLPRI;

  /* Setup the file descriptor for stdout for monitoring. */
  pollfds_.push_front({ stdout, events, 0 });
  map_[stdout] = { runningPos, pollfds_.begin(), true };

  /* Setup the file descriptor for stderr for monitoring. */
  pollfds_.push_front({ stderr, events, 0 });
  map_[stderr] = { runningPos, pollfds_.begin(), false };
}

void PosixSubProcessManager::run() {
  sigset_t set;
  sigemptyset(&set);

  VectorFds fds(pollfds_.begin(), pollfds_.end());

  if (ppoll(&fds.front(), fds.size(), NULL, &set) < 0) {
    THROW_ERROR(errno, "ppol failed");
  }

  /* Try to read data from each fd that is ready. */
  for (auto it = fds.begin(); it != fds.end(); ++it) {
    if (it->revents) {
      readFd(it->fd);
    }
  }
}

PosixSubProcessPtr PosixSubProcessManager::waitForNext() {
  while (finished_.empty()) {
    run();
  }
  PosixSubProcessPtr proc = std::move(finished_.front());
  finished_.pop();
  proc->waitFinished();
  return proc;
}

void PosixSubProcessManager::readFd(int fd) {
  Map::iterator itMap = map_.find(fd);
  assert(itMap != map_.end());

  size_t runningPos = itMap->second.runningPos;
  PosixSubProcessPtr& proc = running_[runningPos];

  bool fdDone = itMap->second.isStdout
    ? proc->readStdout() : proc->readStderr();

  if (fdDone) {
    /* This file descriptor is closed, remove the corresponding entries from
     * map_ and pollfds_. */
    pollfds_.erase(itMap->second.itFd);
    map_.erase(itMap);

    if (proc->completed()) {
      /* Both stdout and stderr fds were closed. The process is complete so we
       * move it from running_ to finished_. */
      finished_.push(std::move(proc));
      running_.erase(running_.begin() + runningPos);
    }
  }
}

} // namespace falcon
