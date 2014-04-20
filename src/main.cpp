/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "options.h"
#include "daemon_instance.h"
#include "exceptions.h"

static void set_options(std::unique_ptr<falcon::Options>& opt) {
  opt->addOption("daemon,d", "daemonize the build system");
  opt->addOption("build,b", "launch a sequential build");
  opt->addOption("config,f", po::value<std::string>(), "falcon configuration file");
  opt->addOption("module,M", po::value<std::string>(), "use -M help for more info");
}

static int load_module(std::unique_ptr<falcon::Graph> g, std::string const& s) {

  if (0 == s.compare("graph")) {
    falcon::GraphGraphizPrinter ggp;
    g->accept(ggp);
  } else if (0 == s.compare("make")) {
    falcon::GraphMakefilePrinter gmp;
    g->accept(gmp);
  } else if (0 == s.compare("help")) {
    std::cout << "list of available modules: " << std::endl
      << "  graph  show the graph in DOT format" << std::endl
      << "  make   show the graph in Makefile format" << std::endl;
  } else {
    std::cerr << "module not supported: " << s << std::endl;
    return 1;
  }

  return 0;
}

int main (int const argc, char const* const* argv)
{
  std::unique_ptr<falcon::Options> optptr (new falcon::Options());

  /* set the falcon's options */
  set_options(optptr);

  /* parse the command line options */
  try {
    optptr->parseOptions(argc, argv);
  } catch (falcon::Exception e) {
    if (e.getCode () != 0) { // the help throw a SUCCESS code, no error to show
      std::cerr << e.getErrorMessage() << std::endl;
    }
    return e.getCode();
  }

  /* Analyze the graph given in the configuration file */
  falcon::GraphParser graphParser;
  std::string configFile = optptr->isOptionSetted("config")
                         ? optptr->vm_["config"].as<std::string>()
                         : "makefile.json";
  try {
    graphParser.processFile(configFile);
  } catch (falcon::Exception e) {
    std::cerr << e.getErrorMessage () << std::endl;
    return e.getCode();
  }

  /* if a module has been requested to execute then load it and return */
  if (optptr->isOptionSetted("module")) {
    return load_module(graphParser.getGraph(),
                       optptr->vm_["module"].as<std::string>());
  }

  /* by default, falcon perform a sequential build.
   * user has to explicitely request to launch it using a daemon */
  falcon::DaemonInstance daemon;
  daemon.loadConf(graphParser.getGraph());
  if (!optptr->isOptionSetted("daemon") || optptr->isOptionSetted("build")) {
    daemon.startBuild();
  } else {
    daemon.start();
  }

  return 0;
}
