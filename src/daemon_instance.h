/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DAEMON_INSTANCE_H_
#define FALCON_DAEMON_INSTANCE_H_

#include <memory>
#include <mutex>
#include <thread>

#include "FalconService.h"
#include "graph.h"
#include "graph_sequential_builder.h"
#include "graphparser.h"
#include "options.h"
#include "stream_server.h"

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

  /**
   * Entry point of the stream server's thread.
   */
  void streamServerThread();

  /* Commands.
   * See thrift/FalconService.thrift for a description of these commands. */

  StartBuildResult::type startBuild();
  FalconStatus::type getStatus();
  void getDirtySources(std::set<std::basic_string<char>>& sources);
  void setDirty(const std::string& target);
  void interruptBuild();
  void shutdown();

 private:

  void onBuildCompleted(BuildResult res);

  unsigned int buildId_;

  std::unique_ptr<Graph> graph_;
  std::unique_ptr<GlobalConfig> config_;
  std::thread serverThread_;

  std::unique_ptr<IGraphBuilder> builder_;

  bool isBuilding_;

  /* Mutex to protect concurrent access to graph_ and isBuilding_. */
  std::mutex mutex_;
  typedef std::lock_guard<std::mutex> lock_guard;

  std::thread streamServerThread_;
  StreamServer streamServer_;
};

} // namespace falcon

#endif // falcon_daemon_instance_h_

