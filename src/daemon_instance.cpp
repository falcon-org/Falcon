/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "daemon_instance.h"

#include "graphparser.h"
#include "server.h"

#include "watchman.h"
#include "exceptions.h"
#include "logging.h"

using namespace std::placeholders;

namespace falcon {

DaemonInstance::DaemonInstance(std::unique_ptr<GlobalConfig> gc)
    : buildId_(0), config_(std::move(gc)), isBuilding_(false),
      streamServer_() {

}

void DaemonInstance::loadConf(std::unique_ptr<Graph> gp) {
  graph_ = std::move(gp);
}

void DaemonInstance::start() {
  if (config_->runSequentialBuild()) {
    StdoutStreamConsumer consumer;
    GraphSequentialBuilder builder(*graph_.get(),
                                   config_->getWorkingDirectoryPath(),
                                   &consumer);
    builder.startBuild(graph_->getRoots(), false /* No callback. */);
    builder.wait();
    /* TODO: get build exit status. */
    return;
  }

  assert(!server_);

  { /* register to watchman every files */
    try {
      WatchmanServer watchmanServer(config_->getWorkingDirectoryPath());
      watchmanServer.startWatching(graph_.get());
    } catch (falcon::Exception e) {
      LOG(fatal) << e.getErrorMessage();
    }
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
  builder_->wait();
  streamServer_.stop();
  streamServerThread.join();
}

/* Commands */

StartBuildResult::type DaemonInstance::startBuild() {
  lock_guard g(mutex_);

  if (isBuilding_) {
     return StartBuildResult::BUSY;
  }

  isBuilding_ = true;

  streamServer_.newBuild(buildId_);

  builder_.reset(
      new GraphSequentialBuilder(*graph_.get(),
                                 config_->getWorkingDirectoryPath(),
                                 &streamServer_));
  builder_->startBuild(graph_->getRoots(),
      std::bind(&DaemonInstance::onBuildCompleted, this, _1));

  return StartBuildResult::OK;
}

void DaemonInstance::onBuildCompleted(BuildResult res) {
  lock_guard g(mutex_);

  isBuilding_ = false;

  streamServer_.endBuild(res);
  ++buildId_;

  LOG(info) << "Build completed. Status: " << toString(res);
}

FalconStatus::type DaemonInstance::getStatus() {
  lock_guard g(mutex_);

  return isBuilding_ ? FalconStatus::BUILDING : FalconStatus::IDLE;
}

void DaemonInstance::interruptBuild() {
  lock_guard g(mutex_);

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

  LOG(debug) << target << " is marked dirty.";

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

} // namespace falcon
