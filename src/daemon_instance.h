/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DAEMON_INSTANCE_H_
#define FALCON_DAEMON_INSTANCE_H_

#include <memory>

#include "graph.h"
#include "graphparser.h"

namespace falcon {

/**
 * Instance of the Falcon Daemon.
 */
class DaemonInstance {
 public:
  DaemonInstance();

  /**
   * load a new graph
   */
  void loadConf(std::unique_ptr<Graph> g);

  /**
   * Start the daemon.
   */
  void start();

  /**
   * Start a sequential build
   */
  void startBuild();

 private:

  std::unique_ptr<Graph> graph_;
};

} // namespace falcon

#endif // FALCON_DAEMON_INSTANCE_H_

