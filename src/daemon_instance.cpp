/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <sstream>

#include "daemon_instance.h"

#include "command_server.h"
#include "exceptions.h"
#include "graph_consistency_checker.h"
#include "graphparser.h"
#include "logging.h"
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
  assert(!commandServer_);
  assert(graph_);

  /* Register all source files to watchman. */
  try {
    watchmanClient_.watchGraph(*graph_);
  } catch (falcon::Exception& e) {
    LOG(ERROR) << e.getErrorMessage();
  }

  /* Open the stream server's socket and accept clients in another thread. */
  streamServer_.openPort(config_->getNetworkStreamPort());
  std::thread streamServerThread = std::thread(&StreamServer::run,
                                               &streamServer_);

  /* Start the server. This will block until the server shuts down. */
  LOG(INFO) << "Starting server...";
  commandServer_.reset(new CommandServer(this, config_->getNetworkAPIPort()));
  commandServer_->start();

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

  LOG(INFO) << "Build completed. Status: " << toString(res);
}

FalconStatus::type DaemonInstance::getStatus() {
  return isBuilding_ ? FalconStatus::BUILDING : FalconStatus::IDLE;
}

void DaemonInstance::interruptBuild() {
  LOG(INFO) << "Interrupting build.";
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

void DaemonInstance::getDirtyTargets(std::set<std::string>& targets) {
  lock_guard g(mutex_);

  NodeMap& nodes = graph_->getNodes();
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->second->getState() == State::OUT_OF_DATE) {
      targets.insert(it->second->getPath());
    }
  }
}

void DaemonInstance::getInputsOf(std::set<std::string>& inputs,
                                 const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  Rule* rule = it->second->getChild();
  if (rule != nullptr) {
    auto ins = rule->getInputs();
    for (auto itInput = ins.begin(); itInput != ins.end(); ++itInput) {
      inputs.insert((*itInput)->getPath());
    }
  }
}

void DaemonInstance::getOutputsOf(std::set<std::string>& outputs,
                                  const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  auto rules = it->second->getParents();
  for (auto itRule = rules.begin(); itRule != rules.end(); ++itRule){
    auto outs = (*itRule)->getOutputs();
    for (auto itOutput = outs.begin(); itOutput != outs.end(); ++itOutput) {
      outputs.insert((*itOutput)->getPath());
    }
  }
}

void DaemonInstance::getHashOf(std::string& hash, const std::string& target) {
  lock_guard g(mutex_);

  auto it = graph_->getNodes().find(target);
  if (it == graph_->getNodes().end()) {
    throw TargetNotFound();
  }

  hash = it->second->getHash();
}

void DaemonInstance::setDirty(const std::string& target) {
  lock_guard g(mutex_);

  /* Find the target. */
  auto& map = graph_->getNodes();
  auto it = map.find(target);
  if (it == map.end()) {
    throw TargetNotFound();
  }

  /* Mark the node and all its outputs dirty, recompute the hashes. */
  it->second->markDirty();
}

void DaemonInstance::shutdown() {
  LOG(INFO) << "Shutting down.";

  /* Interrupt the current build. */
  interruptBuild();

  /* Stop the thrift server. */
  assert(commandServer_);
  /* TODO: is this a problem to call stop from a thrift handler? */
  commandServer_->stop();
}

void DaemonInstance::getGraphviz(std::string& str) {
  lock_guard g(mutex_);

  assert(graph_);
  std::ostringstream oss;
  printGraphGraphiz(*graph_, oss);
  str = oss.str();
}

} // namespace falcon
