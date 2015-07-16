/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

#include "exceptions.h"
#include "util/event.h"

namespace falcon {

static inline void set_flags(int fd) {
    int f = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, f | FD_CLOEXEC | O_NONBLOCK);
}

EventNotifier::EventNotifier() {
    int fds[2];

    if (pipe(fds) == -1) {
        THROW_ERROR(errno, "Failed to create EventNotifier");
    }

    rfd_ = fds[0];
    wfd_ = fds[1];

    set_flags(rfd_);
    set_flags(wfd_);
}

EventNotifier::~EventNotifier() {
    if (rfd_ != wfd_) {
        close(rfd_);
    }
    close(wfd_);
}

int EventNotifier::raise() {
    uint64_t value = 1;
    int ret;

    do {
        ret = write(wfd_, &value, sizeof(value));
    } while (ret < 0 && errno == EINTR);

    /* EAGAIN is fine, a read must be pending.  */
    if (ret < 0 && errno != EAGAIN) {
        return -1;
    }

    return 0;
}

void EventNotifier::flush() {
    ssize_t len;
    char buffer[512];

    do {
        len = read(rfd_, buffer, sizeof(buffer));
    } while ((len == -1 && errno == EINTR) || len == sizeof(buffer));
}

}
