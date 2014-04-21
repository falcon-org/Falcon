/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "daemon_instance.h"
#include "graphparser.h"
#include "server.h"

using namespace std::placeholders;

namespace falcon {

DaemonInstance::DaemonInstance(std::unique_ptr<GlobalConfig> gc)
    : config_(std::move(gc)), isBuilding_(false) {
}

void DaemonInstance::loadConf(std::unique_ptr<Graph> gp) {
  graph_ = std::move(gp);
}

void DaemonInstance::start() {
  if (config_->runSequentialBuild()) {
    startBuild();
    return;
  }

  /* Start the server. This will block until the server terminates. */
  std::cout << "Starting server..." << std::endl;
  Server server(this, 4242);
  server.start();
}

/* Commands */

StartBuildResult::type DaemonInstance::startBuild() {
  lock_guard g(mutex_);

  if (isBuilding_) {
     return StartBuildResult::BUSY;
  }

  isBuilding_ = true;

  builder_.reset(new GraphSequentialBuilder(*graph_.get()));
  builder_->startBuild(graph_->getRoots(),
      std::bind(&DaemonInstance::onBuildCompleted, this, _1));

  return StartBuildResult::OK;
}

void DaemonInstance::onBuildCompleted(BuildResult res) {
  lock_guard g(mutex_);

  isBuilding_ = false;
  std::cout << "Build completed" << std::endl;

  /* TODO. */
  (void)res;
}

FalconStatus::type DaemonInstance::getStatus() {
  lock_guard g(mutex_);

  return isBuilding_ ? FalconStatus::BUILDING
                     : FalconStatus::IDLE;
}

void DaemonInstance::interruptBuild() {
  lock_guard g(mutex_);

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

  std::cout << "set " << target << " to be dirty" << std::endl;

  /* Find the target. */
  auto& map = graph_->getNodes();
  auto it = map.find(target);
  if (it == map.end()) {
    std::cout << "could not find " << target << std::endl;
    throw TargetNotFound();
  }
  std::cout << target << " found: " << it->second << std::endl;
  it->second->markDirty();
}

void DaemonInstance::shutdown() {
  interruptBuild();

  /* TODO: stop the server. */
}

} // namespace falcon
