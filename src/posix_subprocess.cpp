/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exceptions.h"
#include "posix_subprocess.h"

namespace falcon {

PosixSubProcess::PosixSubProcess(const std::string& command)
    : command_(command) { }

SubProcessExitStatus PosixSubProcess::start() {
  pid_t pid = fork();
  if (pid < 0) {
    THROW_ERROR(errno, strerror(errno));
  }

  if (pid == 0) {
    /* Child process. */
      execl("/bin/sh", "/bin/sh", "-c", command_.c_str(), (char *) NULL);
  }

  /* Parent process. Wait for the child to finish. */

  int status;
  if (waitpid(pid, &status, 0) < 0) {
    THROW_ERROR(errno, strerror(errno));
  }

  if (WIFEXITED(status)) {
    int r = WEXITSTATUS(status);
    if (r == 0)
      return SubProcessExitStatus::SUCCEEDED;
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT)
      return SubProcessExitStatus::INTERRUPTED;
  }
  return SubProcessExitStatus::FAILED;
}

} // namespace falcon
