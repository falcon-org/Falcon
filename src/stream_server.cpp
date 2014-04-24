/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <algorithm>
#include <cassert>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "stream_server.h"

#include "exceptions.h"
#include "logging.h"

namespace falcon {

StreamServer::StreamServer(unsigned int port) {
  serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket_ < 0) {
    THROW_ERROR(errno, "Failed to open socket");
  }

  int on = 1;
  if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &on,
                 sizeof(on)) < 0) {
    close(serverSocket_);
    THROW_ERROR(errno, "setsockopt");
  }

  if (ioctl(serverSocket_, FIONBIO, &on) < 0) {
    close(serverSocket_);
    THROW_ERROR(errno, "ioctl");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(serverSocket_);
    THROW_ERROR(errno, "bind");
  }

  if (listen(serverSocket_, 32) < 0) {
    close(serverSocket_);
    THROW_ERROR(errno, "listen");
  }

  eventFd_ = eventfd(0, 0);
  if (eventFd_ < 0) {
    close(serverSocket_);
    THROW_ERROR(errno, "eventfd");
  }

  /* monitor the server socket for read so that we can now when it is ready to
   * accept. */
  fds_.push_back({ serverSocket_, POLLIN });

  /* monitor the event fd so that poll can be notified when the set of fds to
   * monitor has changed. */
  fds_.push_back({ eventFd_, POLLIN });
}

StreamServer::~StreamServer() {
  std::lock_guard<std::mutex> lock(mutex_);

  /* Make sure all sockets are closed. */
  for (auto it = map_.begin(); it != map_.end(); ++it) {
    closeClient(fds_[it->second.fdPos].fd);
  }

  /* Only the server socket should remain in fds_. Close it. */
  assert(fds_.size() == 2);
  close(eventFd_);
  close(serverSocket_);
}

void StreamServer::run() {
  /* TODO: provide a shutdown mechanism. */
  while (true) {
    processEvents();
  }
}

void StreamServer::processEvents() {
  std::vector<pollfd> fds;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::for_each(fds_.begin(), fds_.end(),
        [&fds](const pollfd& p) {
          fds.push_back(pollfd{p.fd, p.events});
        }
    );
  }

  int r = poll(&fds.front(), fds.size(), -1);
  if (r < 0) {
    if (errno == EINTR) {
      return;
    } else if (errno == EINVAL) {
      /* Too many file descriptors. */
      LOG(error) << "Too many fds for poll";
    }
    THROW_ERROR(errno, "poll failed");
  } else if (r == 0) {
    /* Timeout. */
    return;
  }

  /* Try to read data from each fd that is ready. */
  for (auto it = fds.begin(); it != fds.end(); ++it) {
    if (it->revents == 0) {
      continue;
    }
    if (it->fd == serverSocket_) {
      if (it->revents & POLLIN) {
        acceptClients();
      } else {
        LOG(error) << "Unexpected poll event " << it->revents;
      }
    } else if (it->fd != eventFd_) {
      if (it->revents & POLLOUT) {
        processClient(it->fd);
      } else {
        LOG(error) << "Unexpected poll event " << it->revents;
      }
    } else {
      /* Notified by eventfd. */
      flushEventFd();
    }
  }
}

void StreamServer::flushEventFd() {
  uint64_t buf;
  int r;

  do {
    r = read(eventFd_, &buf, sizeof(uint64_t));
  } while (r == -1 && errno == EINTR);

  if (r != sizeof(uint64_t)) {
    THROW_ERROR(errno, "read");
  }
}

void StreamServer::acceptClients() {
  /* Accept each incoming connection. */
  int clientFd = -1;
  do {
    clientFd = accept(serverSocket_, NULL, NULL);
    if (clientFd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        /* We accepted all the connections. */
        break;
      } else {
        THROW_ERROR(errno, "accept");
      }
    } else {
      createClient(clientFd);
    }
  } while (clientFd != -1);
}

void StreamServer::createClient(int fd) {
  std::lock_guard<std::mutex> lock(mutex_);
  bool isWaiting = buf_.size() == 0;
  size_t fdPos;

  if (isWaiting) {
    waiting_.push_back(fd);
    fdPos = waiting_.size() - 1;
  } else {
    fds_.push_back(pollfd{ fd, POLLOUT });
    fdPos = fds_.size() - 1;
  }

  map_[fd] = ClientInfo{0, fdPos, isWaiting};
}

void StreamServer::removeFd(size_t fdPos) {
  assert(fdPos < fds_.size());
  pollfd fdLast = fds_.back();
  fds_[fdPos] = fdLast;
  map_[fdLast.fd].fdPos = fdPos;
  fds_.pop_back();
}

void StreamServer::processClient(int fd) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = map_.find(fd);
  assert(it != map_.end());
  ClientInfo& info = it->second;

  /* There has to be some data to be written, otherwise this fd should have been
   * on the waiting_ list. */
  assert(info.bufPtr < buf_.size());

  size_t bufSize = buf_.size() - info.bufPtr;
  const char* bufPtr = &buf_[info.bufPtr];
  do {
    int r = send(fd, bufPtr, bufSize, MSG_NOSIGNAL);
    if (r < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        closeClient(fd);
      }
      return;
    }
    info.bufPtr += r;
    bufSize -= r;
  } while (info.bufPtr < buf_.size());

  /* If we reach here, it means there is nothing left to write for this
   * client. Put it in the waiting list. */
  removeFd(it->second.fdPos);
  waiting_.push_back(fd);
  it->second.isWaiting = true;
  it->second.fdPos = waiting_.size() - 1;
}

void StreamServer::closeClient(int fd) {
  auto itMap = map_.find(fd);
  assert(itMap != map_.end());

  removeFd(itMap->second.fdPos);
  map_.erase(itMap);

  close(fd);
}

void StreamServer::writeBuf(char* buf, size_t len) {
  {
    std::lock_guard<std::mutex> lock(mutex_);

    buf_.append(buf, len);

    /* We have new data, flush the waiting_ list. */
    for (auto it = waiting_.begin(); it != waiting_.end(); ++it) {
      fds_.push_back(pollfd{ *it, POLLOUT });
      auto itMap = map_.find(*it);
      assert(itMap != map_.end());
      itMap->second.fdPos = fds_.size() - 1;
      itMap->second.isWaiting = false;
    }
    waiting_.clear();
  }

  /* Notifiy poll that fds_ changed by writting to eventFd_. */
  uint64_t u = 1;
  int r = write(eventFd_, &u, sizeof(uint64_t));
  if (r != sizeof(uint64_t)) {
    THROW_ERROR(errno, "write");
  }
}

void StreamServer::writeStdout(unsigned int cmdId, char* buf, size_t len) {
  (void)cmdId;
  writeBuf(buf, len);
}

void StreamServer::writeStderr(unsigned int cmdId, char* buf, size_t len) {
  (void)cmdId;
  writeBuf(buf, len);
}

} // namespace falcon
