/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_SERVER_H_
#define FALCON_SERVER_H_

#include "FalconService.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace falcon {

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::Test;

class FalconServiceHandler : virtual public FalconServiceIf {
 public:
  FalconServiceHandler();
  int64_t getPid();
  void startBuild();
  void shutdown();
};

class Server {
 public:
  explicit Server(int port);
  void start();

 private:
  boost::shared_ptr<FalconServiceHandler> handler_;
  boost::shared_ptr<TProcessor> processor_;
  boost::shared_ptr<TServerTransport> serverTransport_;
  boost::shared_ptr<TTransportFactory> transportFactory_;
  boost::shared_ptr<TProtocolFactory> protocolFactory_;

  std::unique_ptr<TSimpleServer> server_;

  Server(const Server& other) = delete;
  Server& operator=(const Server&) = delete;
};

} // namespace falcon

#endif // FALCON_SERVER_H_

