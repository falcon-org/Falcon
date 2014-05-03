/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "watchman.h"
#include "exceptions.h"
#include "logging.h"
#include "json/parser.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#if !defined(UNIX_PATH_MAX)
# define UNIX_PATH_MAX 108
#endif

#include "posix_subprocess.h"

namespace falcon {

/* ************************************************************************* */
/*                           Watchman Server                                 */
/* ************************************************************************* */

WatchmanClient::WatchmanClient(std::string const& workingDirectory)
  : workingDirectory_(workingDirectory)
  , isConnected_(false)
  , watchmanSocket_(-1)
{
  socketPath_ = workingDirectory + "/.watchman.socket";
  logPath_ = workingDirectory + "/.watchman.log";
  statePath_ = workingDirectory + "/.watchman.state";
}

WatchmanClient::~WatchmanClient() {
  if (watchmanSocket_ >= 0) {
    close(watchmanSocket_);
  }
}

void WatchmanClient::connectToWatchman() {
  assert(!isConnected_);

  /* Try to open the watchman socket */
  try {
    openWatchmanSocket();
  } catch (Exception& e) {
    /* If can't connect to watchman, it's probably not running, so try to spawn
     * it and retry. */
    startWatchmanInstance();
    openWatchmanSocket();
  }

  /* If we end up here, we should have been able to connect. */
  assert(isConnected_);
}

void WatchmanClient::startWatchmanInstance() {
  std::stringstream ss;

  ss << "watchman"
     << " --sockname=\"" << socketPath_ << "\""
     << " --logfile=\"" << logPath_ << "\"";

  std::unique_ptr<StdoutStreamConsumer> consumer(new StdoutStreamConsumer());
  PosixSubProcessManager manager(consumer.get());
  manager.addProcess(ss.str(), workingDirectory_);
  PosixSubProcessPtr p = manager.waitForNext();

  auto status = p->status();
  if (status != SubProcessExitStatus::SUCCEEDED) {
    LOG(ERROR) << "can't start watchman";
  }
}

void WatchmanClient::openWatchmanSocket() {
  assert(!isConnected_);
  struct sockaddr_un addr;

  watchmanSocket_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (watchmanSocket_ < 0) {
    THROW_ERROR(errno, "unable to open the watchman's socket");
  }

  memset(&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", socketPath_.c_str());

  if (connect(watchmanSocket_, (struct sockaddr*)&addr,
              sizeof (struct sockaddr_un)) != 0) {
    THROW_ERROR(errno, "unable to connect to the watchman's socket");
  }

  isConnected_ = true;
}

void WatchmanClient::watchGraph(const Graph& g) {
  if (!isConnected_) {
    connectToWatchman();
  }

  auto nodeMap = g.getNodes();
  for (auto it = nodeMap.cbegin(); it != nodeMap.cend(); it++) {
    assert(it->second);
    watchNode(*it->second);
  }
}

void WatchmanClient::watchNode(const Node& n) {
  if (!isConnected_) {
    connectToWatchman();
  }

  /* TODO: * For now, we only watch source files, ie files that are not
   * generated. If we were to watch the generated targets, we'd be notified
   * each time a target is built. */
  if (n.getChild() != nullptr) {
    return;
  }

  std::string targetPattern(n.getPath());
  std::string targetDirectory = workingDirectory_;

  { /* Clean the target Pattern and the target directory */
    if (targetPattern.compare(0, 3, "../") == 0) {
      /* in case of a relative path: example:
       * n.getPath():       "../../src/main.cpp",
       * workingDirectory_: "/home/nicolas/falcon/build/v1.0.1"
       * targetDirectory:   "/home/nicolas/falcon"
       * targetPattern:     "src/main.cpp" */
      while (targetPattern.compare(0, 3, "../") == 0) {
        targetPattern = targetPattern.substr(3);
        unsigned pos = targetDirectory.find_last_of("/");
        targetDirectory = targetDirectory.substr(0, pos);
        targetDirectory.resize(pos);
      }
    } else if (targetPattern[0] == '/') {
      /* in case of a full path, example:
       * n.getPath():     "/usr/local/lib/libboost_program_options.so",
       * targetDirectory: "/usr/local/lib"
       * targetPattern:   "libboost_program_options.so" */
      unsigned pos = targetPattern.find_last_of("/");
      targetDirectory = targetPattern.substr(0, pos);
      targetDirectory.resize(pos);
      unsigned size = targetPattern.size();
      targetPattern = targetPattern.substr(pos+1);
      targetPattern.resize(size - pos - 1);
    }
  }

  std::stringstream ss;
  ss << "[ ";
  /* Set the command request: subscribe */
  ss << "\"trigger\"";
  /* Set the directory to watch in */
  ss << ", \"" << targetDirectory << "\"";
  /* the trigger Name */
  ss << ", \"" << n.getPath() << "\"";
  /* Set the file name (the exact file name) to watch (should be in the working
   * directory tree) */
  ss << ", \"" << targetPattern << "\""
     << ", \"--\", \"falcon.py\", \"-d\", \"" << n.getPath() << "\" ]\n";

  /* Send the command to watchman */
  std::string cmd = ss.str();
  try {
    writeCommand(cmd);
  } catch (Exception& e) {
    /* We may have lost the connection, reconnect and try again. */
    connectToWatchman();
    writeCommand(cmd);
  }

  /* Get watchman return value */
  readAnswer();
}

void WatchmanClient::writeCommand(std::string const& cmd) {
  int r = 0;
  for (unsigned int i = 0; i != cmd.size(); i += r) {
    r = write(watchmanSocket_, &cmd[i], cmd.size() - i);
    if (r == -1) {
      LOG(ERROR) << "werror while writing command to watchman socket: "
                 << "errno(" << errno << ") " << strerror(errno);
      if (errno == EAGAIN) {
        r = 0;
      } else {
        close(watchmanSocket_);
        isConnected_ = false;
        THROW_ERROR(errno, "unable to write command to watchman");
      }
    }
  }
  DLOG(INFO) << "[WATCHMAN] <--- " << cmd;
}

void WatchmanClient::readAnswer() {
  JsonParser parser;
  JsonVal* dom;
#define MAX_JSON_STRING_SIZE 1024
#define MAX_BUF_SIZE         256
  char buf[MAX_BUF_SIZE];
  int loop = 0; /* a loop counter to avoid reading garbage indefinitely */

  do {
    int r = read(watchmanSocket_, buf, sizeof(buf) - 1);
    if (r == -1) {
      if (errno == EAGAIN) { loop++; continue; }
      LOG(ERROR) << "werror while reading on watchman socket: "
                  << "errno(" << errno << ") " << strerror(errno);
      close(watchmanSocket_);
      isConnected_ = false;
      THROW_ERROR(errno, "unable to read watchman command response");
      return;
    }
    parser.parse(0, buf, r);
    dom = parser.getDom();
    DLOG(INFO) << "[WATCHMAN] ---> " << buf;
  } while (!dom && ((++loop) < (MAX_JSON_STRING_SIZE / MAX_BUF_SIZE)));

  if (!dom) {
    THROW_ERROR(EINVAL, "Error parsing watchman command response");
  }
  /* In case of an error: */
  JsonVal const* error = dom->getObject("error");
  if (error) {
    LOG(ERROR) << error->_data;
  }
}

} // namespace falcon