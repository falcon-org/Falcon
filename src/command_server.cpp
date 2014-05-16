/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <cassert>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "command_server.h"

#include "daemon_instance.h"

namespace falcon {

FalconServiceHandler::FalconServiceHandler(DaemonInstance* daemon)
    : daemon_(daemon) {
  assert(daemon != nullptr);
}

int64_t FalconServiceHandler::getPid() {
  return getpid();
}

StartBuildResult::type FalconServiceHandler::startBuild(
    const std::set<std::string>& targets, int32_t numThreads, bool lazyFetch) {
  return daemon_->startBuild(targets, numThreads, lazyFetch);
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

void FalconServiceHandler::getDirtyTargets(std::set<std::string>& targets) {
  daemon_->getDirtyTargets(targets);
}

void FalconServiceHandler::getInputsOf(std::set<std::string>& inputs,
                                       const std::string& target) {
  daemon_->getInputsOf(inputs, target);
}

void FalconServiceHandler::getOutputsOf(std::set<std::string>& outputs,
                                        const std::string& target) {
  daemon_->getOutputsOf(outputs, target);
}

void FalconServiceHandler::getHashOf(std::string& hash,
                                     const std::string& target) {
  daemon_->getHashOf(hash, target);
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

CommandServer::CommandServer(DaemonInstance* daemon, int port) {
  handler_.reset(new FalconServiceHandler(daemon));
  processor_.reset(new FalconServiceProcessor(handler_));
  serverTransport_.reset(new TServerSocket(port));
  transportFactory_.reset(new TBufferedTransportFactory());
  protocolFactory_.reset(new TBinaryProtocolFactory());

  server_.reset(new TThreadedServer(processor_, serverTransport_,
                                    transportFactory_, protocolFactory_));
}

void CommandServer::start() {
  server_->serve();
}

void CommandServer::stop() {
  server_->stop();
}

} // namespace falcon

