/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "daemon_instance.h"
#include "graph_sequential_builder.h"
#include "graphparser.h"

namespace falcon {

DaemonInstance::DaemonInstance() {
}

void DaemonInstance::loadConf(const std::string& confPath) {
  GraphParser graphParser;
  graphParser.processFile(confPath);
  graph_ = graphParser.getGraph();
}

void DaemonInstance::start() {
  /* TODO: start monitoring source files with watchman. */
  /* TODO: start accepting client connections. */

#if !defined(NDEBUG)
  GraphGraphizPrinter gpp;
  gpp.visit(*graph_);
#endif

  startBuild();
}

void DaemonInstance::startBuild() {
  std::cout << "building..." << std::endl;
  GraphSequentialBuilder builder(*graph_.get());
  builder.build(graph_->getRoots());
}

} // namespace falcon
