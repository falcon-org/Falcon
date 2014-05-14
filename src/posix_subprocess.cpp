/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include <cassert>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "exceptions.h"
#include "posix_subprocess.h"

#include "logging.h"

namespace falcon {

std::string toString(SubProcessExitStatus v) {
  switch (v) {
    case SubProcessExitStatus::UNKNOWN: return "UNKNOWN";
    case SubProcessExitStatus::SUCCEEDED: return "SUCCEEDED";
    case SubProcessExitStatus::INTERRUPTED: return "INTERRUPTED";
    case SubProcessExitStatus::FAILED: return "FAILED";
    default: THROW_ERROR(EINVAL, "Unrecognized SubProcessExitStatus");
  }
}

PosixSubProcess::PosixSubProcess(const std::string& command,
                                 const std::string& workingDirectory,
                                 unsigned int id,
                                 IStreamConsumer* consumer)
  : id_(id), command_(command)
  , workingDirectory_(workingDirectory)
  , stdoutFd_(-1), stderrFd_(-1)
  , consumer_(consumer), pid_(-1), status_(SubProcessExitStatus::UNKNOWN) { }

void PosixSubProcess::start() {

  DLOG(INFO) << "New command: ID = " << id_ << ", CMD = " << command_;

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

void PosixSubProcess::interrupt() {
  assert(pid_ >= 0);
  if (kill(pid_, SIGINT) < 0) {
    LOG(ERROR) << "Error, kill: " << strerror(errno);
  }
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

      DLOG(FATAL) << "unable to set the working directory before executing "
                      "the command";
      THROW_ERROR_CODE(errno);
    }

    /* Run the command. */
    execl("/bin/sh", "/bin/sh", "-c", command_.c_str(), (char *) NULL);
  } while(false);

  /* If we get here, something failed. */
  LOG(FATAL) << "Error, failed to execute command in child process";
  _exit(1);
}

bool PosixSubProcess::completed() {
  return stdoutFd_ == -1 && stderrFd_ == -1;
}

bool PosixSubProcess::onFdReady(int& fd, bool isStdout) {
  assert(fd != -1);
  char buf[4 << 10];
  ssize_t len = read(fd, buf, sizeof(buf));
  if (len > 0) {
    if (consumer_ != nullptr) {
      if (isStdout) {
        consumer_->writeStdout(id_, buf, len);
      } else {
        consumer_->writeStderr(id_, buf, len);
      }
    }
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

  status_ = SubProcessExitStatus::FAILED;
  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status) == 0) {
      status_ = SubProcessExitStatus::SUCCEEDED;
    }
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT) {
      status_ = SubProcessExitStatus::INTERRUPTED;
    }
  }

  DLOG(INFO) << "Completed command: ID = " << id_ << ", STATUS = "
    << toString(status_);
}

} // namespace falcon
