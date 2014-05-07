/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DAEMON_INSTANCE_H_
#define FALCON_DAEMON_INSTANCE_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "FalconService.h"
#include "build_plan.h"
#include "command_server.h"
#include "graph.h"
#include "graph_builder.h"
#include "graphparser.h"
#include "options.h"
#include "stream_server.h"
#include "watchman.h"

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

  Graph& getGraph() const { assert(graph_); return *graph_; }

  /* Commands.
   * See thrift/FalconService.thrift for a description of these commands. */

  StartBuildResult::type startBuild(int32_t numThreads);
  FalconStatus::type getStatus();
  void getDirtySources(std::set<std::basic_string<char>>& sources);
  void getDirtyTargets(std::set<std::basic_string<char>>& targets);
  void getInputsOf(std::set<std::string>& inputs, const std::string& target);
  void getOutputsOf(std::set<std::string>& outputs, const std::string& target);
  void getHashOf(std::string& hash, const std::string& target);
  void setDirty(const std::string& target);
  void interruptBuild();
  void shutdown();
  void getGraphviz(std::string& str);

 private:

  void onBuildCompleted(BuildResult res);

  /** Wait for the current build to complete. */
  void waitForBuild();

  /** Check if any source file is missing. Throw an exception of type
   * InvalidGraphError if it is the case. */
  void checkSourcesMissing();

  unsigned int buildId_;

  std::unique_ptr<Graph> graph_;
  std::unique_ptr<GlobalConfig> config_;
  std::thread serverThread_;

  std::unique_ptr<BuildPlan> plan_;
  std::unique_ptr<IGraphBuilder> builder_;

  WatchmanClient watchmanClient_;

  std::atomic_bool isBuilding_;

  /* Mutex to protect concurrent access to graph_. */
  std::mutex mutex_;
  typedef std::lock_guard<std::mutex> lock_guard;

  StreamServer streamServer_;

  /* The thrift server. */
  std::unique_ptr<CommandServer> commandServer_;

  /**
   * When watchman notifies us of a file change, we stat it. If the file does
   * not exist and is a source file, we add it here. Each time we build, we
   * check if there is a missing source file and return an error instead of
   * building.
   */
  NodeSet sourcesMissing_;
};

} // namespace falcon

#endif // falcon_daemon_instance_h_
