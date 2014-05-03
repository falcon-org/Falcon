/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <sstream>

#include "daemon_instance.h"

#include "exceptions.h"
#include "graph_consistency_checker.h"
#include "graphparser.h"
#include "logging.h"
#include "server.h"
#include "watchman.h"

using namespace std::placeholders;

namespace falcon {

DaemonInstance::DaemonInstance(std::unique_ptr<GlobalConfig> gc)
    : buildId_(0)
    , config_(std::move(gc))
    , watchmanClient_(config_->getWorkingDirectoryPath())
    , isBuilding_(false)
    , streamServer_() { }

void DaemonInstance::loadConf(std::unique_ptr<Graph> gp) {
  graph_ = std::move(gp);
}

void DaemonInstance::start() {
  assert(!config_->runSequentialBuild());
  assert(!server_);
  assert(graph_);

  /* Register all source files to watchman. */
  try {
    watchmanClient_.watchGraph(*graph_);
  } catch (falcon::Exception& e) {
    LOG(fatal) << e.getErrorMessage();
  }

  /* Open the stream server's socket and accept clients in another thread. */
  streamServer_.openPort(config_->getNetworkStreamPort());
  std::thread streamServerThread = std::thread(&StreamServer::run,
                                               &streamServer_);

  /* Start the server. This will block until the server shuts down. */
  LOG(info) << "Starting server...";
  server_.reset(new Server(this, config_->getNetworkAPIPort()));
  server_->start();

  /* If we reach here, the server was shut down. */
  if (builder_) {
    builder_->wait();
  }
  streamServer_.stop();
  streamServerThread.join();
}

/* Commands */

StartBuildResult::type DaemonInstance::startBuild() {
  assert(graph_);

  if (isBuilding_.load(std::memory_order_acquire)) {
     return StartBuildResult::BUSY;
  }

  FALCON_CHECK_GRAPH_CONSISTENCY(graph_.get(), mutex_);

  isBuilding_.store(true, std::memory_order_release);

  streamServer_.newBuild(buildId_);

  builder_.reset(
      new GraphSequentialBuilder(*graph_, mutex_,
                                 &watchmanClient_,
                                 config_->getWorkingDirectoryPath(),
                                 &streamServer_));

  /* Build all the roots by default. */
  builder_->startBuild(graph_->getRoots(),
      std::bind(&DaemonInstance::onBuildCompleted, this, _1));

  return StartBuildResult::OK;
}

void DaemonInstance::onBuildCompleted(BuildResult res) {
  assert(isBuilding_);
  streamServer_.endBuild(res);

  FALCON_CHECK_GRAPH_CONSISTENCY(graph_.get(), mutex_);

  ++buildId_;
  isBuilding_.store(false, std::memory_order_release);

  LOG(info) << "Build completed. Status: " << toString(res);
}

FalconStatus::type DaemonInstance::getStatus() {
  return isBuilding_ ? FalconStatus::BUILDING : FalconStatus::IDLE;
}

void DaemonInstance::interruptBuild() {
  LOG(info) << "Interrupting build.";
  if (builder_) {
    builder_->interrupt();
  }
}

void DaemonInstance::getDirtySources(std::set<std::string>& sources) {
  lock_guard g(mutex_);

  NodeSet& src = graph_->getSources();
  for (auto it = src.begin(); it != src.end(); ++it) {
    if ((*it)->getState() == State::OUT_OF_DATE) {
      sources.insert((*it)->getPath());
    }
  }
}

void DaemonInstance::setDirty(const std::string& target) {
  lock_guard g(mutex_);

  /* Find the target. */
  auto& map = graph_->getNodes();
  auto it = map.find(target);
  if (it == map.end()) {
    throw TargetNotFound();
  }
  it->second->markDirty();
}

void DaemonInstance::shutdown() {
  LOG(info) << "Shutting down.";

  /* Interrupt the current build. */
  interruptBuild();

  /* Stop the thrift server. */
  assert(server_);
  /* TODO: is this a problem to call stop from a thrift handler? */
  server_->stop();
}

void DaemonInstance::getGraphviz(std::string& str) {
  lock_guard g(mutex_);

  assert(graph_);
  std::ostringstream oss;
  falcon::GraphGraphizPrinter ggp(oss);
  graph_->accept(ggp);
  str = oss.str();
}

} // namespace falcon
