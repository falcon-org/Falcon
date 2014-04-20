/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DAEMON_INSTANCE_H_
#define FALCON_DAEMON_INSTANCE_H_

#include <memory>
#include <thread>

#include "graph.h"
#include "graphparser.h"
#include "options.h"

namespace falcon {

/**
 * Instance of the Falcon Daemon.
 */
class DaemonInstance {
 public:
  DaemonInstance(std::unique_ptr<GlobalConfig> gc);

  /**
   * load a new graph
   */
  void loadConf(std::unique_ptr<Graph> g);

  /**
   * Start the daemon.
   */
  void start();

 private:
  /**
   * Start a sequential build
   */
  void startBuild();

  /**
   * Start the server, must be called only once.
   */
  void startServer();

  /**
   * Member function executed by the server thread.
   */
  void serverThread();

  std::unique_ptr<Graph> graph_;
  std::unique_ptr<GlobalConfig> config_;
  std::thread serverThread_;
};

} // namespace falcon

#endif // FALCON_DAEMON_INSTANCE_H_

