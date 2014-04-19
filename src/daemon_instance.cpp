#include "daemon_instance.h"

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
  GraphMakefilePrinter gpp;
  gpp.visit(*graph_);
#endif
}

} // namespace falcon
