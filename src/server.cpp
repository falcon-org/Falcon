/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "server.h"

#include "daemon_instance.h"

namespace falcon {

FalconServiceHandler::FalconServiceHandler(DaemonInstance* daemon)
    : daemon_(daemon) {
  assert(daemon != nullptr);
}

int64_t FalconServiceHandler::getPid() {
  return getpid();
}

StartBuildResult::type FalconServiceHandler::startBuild() {
  return daemon_->startBuild();
}

FalconStatus::type FalconServiceHandler::getStatus() {
  return daemon_->getStatus();
}

void FalconServiceHandler::interruptBuild() {
  daemon_->interruptBuild();
}

void FalconServiceHandler::getDirtySources(std::set<std::string>& sources) {
  daemon_->getDirtySources(sources);
}

void FalconServiceHandler::setDirty(const std::string& target) {
  daemon_->setDirty(target);
}

void FalconServiceHandler::shutdown() {
  daemon_->shutdown();
}

void FalconServiceHandler::getGraphviz(std::string& str) {
  daemon_->getGraphviz(str);
}

Server::Server(DaemonInstance* daemon, int port) {
  handler_.reset(new FalconServiceHandler(daemon));
  processor_.reset(new FalconServiceProcessor(handler_));
  serverTransport_.reset(new TServerSocket(port));
  transportFactory_.reset(new TBufferedTransportFactory());
  protocolFactory_.reset(new TBinaryProtocolFactory());

  server_.reset(new TThreadedServer(processor_, serverTransport_,
                                    transportFactory_, protocolFactory_));
}

void Server::start() {
  server_->serve();
}

void Server::stop() {
  server_->stop();
}

} // namespace falcon

