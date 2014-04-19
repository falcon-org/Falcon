#include "daemon_instance.h"

#include "graphbuilder.h"

namespace falcon {

DaemonInstance::DaemonInstance() {
}

void DaemonInstance::loadConf(const std::string& confPath) {
  GraphBuilder graphBuilder;
  graphBuilder.processFile(confPath);
  graph_ = graphBuilder.getGraph();
}

void DaemonInstance::start() {
  /* TODO: start monitoring source files with watchman. */
  /* TODO: start accepting client connections. */

#if defined(DEBUG)
  GraphMakefilePrinter gpp;
  gpp.visit(*graph_);
#endif
}

} // namespace falcon
