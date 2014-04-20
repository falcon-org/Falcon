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

void FalconServiceHandler::startBuild() {
  std::cout << "Building..." << std::endl;
}

void FalconServiceHandler::shutdown() {
  std::cout << "Shutting down..." << std::endl;
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

