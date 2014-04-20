/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "server.h"

namespace falcon {

FalconServiceHandler::FalconServiceHandler() {
}

int64_t FalconServiceHandler::getPid() {
  return getpid();
}

StartBuildResult::type FalconServiceHandler::startBuild() {
  std::cout << "Building..." << std::endl;

  /* TODO: implement. */

  return StartBuildResult::OK;
}

FalconStatus::type FalconServiceHandler::getStatus() {
  /* TODO: implement. */

  return FalconStatus::IDLE;
}

void FalconServiceHandler::interruptBuild() {
  /* TODO: implement. */

}

void FalconServiceHandler::getDirtySources(
    std::set<std::basic_string<char>>& sources) {
  /* TODO: implement. */
}

void FalconServiceHandler::setDirty(const std::string& target) {
  /* TODO: implement. */
}

void FalconServiceHandler::shutdown() {
  std::cout << "Shutting down..." << std::endl;

  /* TODO: implement. */
}

Server::Server(int port) {
  handler_.reset(new FalconServiceHandler());
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

} // namespace falcon

