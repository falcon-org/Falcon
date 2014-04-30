/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_WATCHMAN_H_
# define FALCON_WATCHMAN_H_

# include "graph.h"

namespace falcon {

/* Watchman Server visitor:
 *
 * Start a watchman dedicated to the working directory.
 * (create a UNIX socket in the working directory, so the instance of watchman
 * will be bound to the instance of the daemon). In the future if we want
 * multiple Falcon daemons working on different projects (and/or sharing the
 * same sources) that will be easier to modify/notify each of these projects.
 *
 * Works in two times:
 *  -1- Construction: instantiate watchman (if needed) and open the socket
 *  -2- register every source file (Visitor method)
 *
 * The actual implementation required watchman to trigger a command to notify
 * Falcon a file changes (actualy, the Falcon client python script with the
 * set dirty option)
 */

class WatchmanServer : public GraphVisitor {
public:
  WatchmanServer(std::string const& workingDirectory);
  ~WatchmanServer();

  /* will to the same as "g->accept(watchmanServerInstance);" */
  void startWatching(Graph* g);

  /* Visitor methods (DO NOT USE, prefer use of startWatching function) */
  virtual void visit(Graph& g);
  virtual void visit(Node& g);
  virtual void visit(Rule& g);

private:
  /* Try to launch watchman with the watchman options (see below) */
  void startWatchmanInstance();
  /* Try to open the UNIX socket to watchman */
  void openWatchmanSocket();

  void writeCommand(std::string const& cmd);
  void readAnswer();

  std::string workingDirectory_;
  int watchmanSocket_; /* UNIX socket */

  /* Watchman option: */
  std::string socketPath_; /* The UNIX socket path */
  std::string logPath_;    /* the watchman's log file */
  std::string statePath_;  /* the watchman's state file (persistent data) */
};
}

#endif /* !FALCON_WATCHMAN_H_ */
