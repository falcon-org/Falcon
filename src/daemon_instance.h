/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DAEMON_INSTANCE_H_
#define FALCON_DAEMON_INSTANCE_H_

#include <memory>

#include "graph.h"

namespace falcon {

/**
 * Instance of the Falcon Daemon.
 */
class DaemonInstance {
 public:
  DaemonInstance();

  /**
   * Load a configuration file and build the internal graph.
   * @param confPath Path where to find the configuration file.
   */
  void loadConf(const std::string& confPath);

  /**
   * Start the daemon.
   */
  void start();

 private:

  void startBuild();

  std::unique_ptr<Graph> graph_;
};

} // namespace falcon

#endif // FALCON_DAEMON_INSTANCE_H_

