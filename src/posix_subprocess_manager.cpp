/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <poll.h>
#include <signal.h>
#include <sys/types.h>

#include "posix_subprocess_manager.h"

#include "logging.h"

namespace falcon {

PosixSubProcessManager::PosixSubProcessManager(IStreamConsumer *consumer)
    : id_(0), consumer_(consumer) { }

PosixSubProcessManager::~PosixSubProcessManager() {
  /* The user should wait for all the processes to complete before
   * destroying this object. */
  assert(nbRunning() == 0);
}

unsigned int PosixSubProcessManager::addProcess(Rule *rule,
    const std::string& workingDirectory) {
  unsigned int id = id_++;
  const std::string& command = rule->getCommand();
  assert(!command.empty());
  PosixSubProcessPtr proc(new PosixSubProcess(command, workingDirectory,
                                              id, consumer_));
  mapToRule_.insert(std::make_pair(proc.get(), rule));
  proc->start();

  int stdout = proc->stdoutFd_;
  int stderr = proc->stderrFd_;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    running_.push_front(std::move(proc));
  }

  short events = POLLIN | POLLPRI;

  /* Setup the file descriptor for stdout for monitoring. */
  pollfds_.push_front({ stdout, events, 0 });
  map_[stdout] = { running_.begin(), pollfds_.begin(), true };

  /* Setup the file descriptor for stderr for monitoring. */
  pollfds_.push_front({ stderr, events, 0 });
  map_[stderr] = { running_.begin(), pollfds_.begin(), false };

  return id;
}

void PosixSubProcessManager::run() {
  sigset_t set;
  sigset_t origmask;
  sigemptyset(&set);

  std::vector<pollfd> fds(pollfds_.begin(), pollfds_.end());

  sigprocmask(SIG_SETMASK, &set, &origmask);
  if (poll(&fds.front(), fds.size(), -1) < 0) {
    THROW_ERROR(errno, "poll failed");
  }
  sigprocmask(SIG_SETMASK, &origmask, NULL);

  /* Try to read data from each fd that is ready. */
  for (auto it = fds.begin(); it != fds.end(); ++it) {
    if (it->revents) {
      readFd(it->fd);
    }
  }
}

PosixSubProcessManager::BuiltRule PosixSubProcessManager::waitForNext() {
  assert(!finished_.empty() || !running_.empty());
  while (finished_.empty()) {
    run();
  }
  PosixSubProcessPtr proc = std::move(finished_.front());
  finished_.pop();
  proc->waitFinished();

  auto it = mapToRule_.find(proc.get());
  assert(it != mapToRule_.end());
  Rule *rule = it->second;
  mapToRule_.erase(it);

  return BuiltRule{rule, proc->status(), proc->id()};
}

void PosixSubProcessManager::interrupt() {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto it = running_.begin(); it != running_.end(); ++it) {
    (*it)->interrupt();
  }
}

void PosixSubProcessManager::readFd(int fd) {
  auto itMap = map_.find(fd);
  assert(itMap != map_.end());

  auto itRunning = itMap->second.itRunning;
  PosixSubProcessPtr& proc = *itRunning;
  assert(proc);

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

      std::lock_guard<std::mutex> lock(mutex_);
      running_.erase(itRunning);
    }
  }
}

} // namespace falcon
