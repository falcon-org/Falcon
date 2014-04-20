/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "daemon_instance.h"
#include "graph_sequential_builder.h"
#include "graphparser.h"
#include "server.h"

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
    return;
  }

  /* Start the server. This will block until the server terminates. */
  startServer();
}

void DaemonInstance::startServer() {
  assert(!serverThread_.joinable());
  serverThread_ = std::thread(&DaemonInstance::serverThread, this);
  serverThread_.join();
}

void DaemonInstance::serverThread() {
  std::cout << "Starting server..." << std::endl;
  Server server(4242);
  server.start();
}

void DaemonInstance::startBuild() {
  std::cout << "building..." << std::endl;
  GraphSequentialBuilder builder(*graph_.get());
  builder.build(graph_->getRoots());
}

} // namespace falcon
