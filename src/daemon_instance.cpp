/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "daemon_instance.h"
#include "graph_sequential_builder.h"
#include "graphparser.h"

namespace falcon {

DaemonInstance::DaemonInstance(std::unique_ptr<GlobalConfig> gc) {
  config_ = std::move(gc);
}

void DaemonInstance::loadConf(std::unique_ptr<Graph> gp) {
  graph_ = std::move(gp);
}

void DaemonInstance::start() {
  if (config_->runSequentialBuild()) {
    startBuild();
  } else {
  /* TODO: start monitoring source files with watchman. */
  /* TODO: start accepting client connections. */

    startBuild();
  }
}

void DaemonInstance::startBuild() {
  std::cout << "building..." << std::endl;
  GraphSequentialBuilder builder(*graph_.get());
  builder.build(graph_->getRoots());
}

} // namespace falcon
