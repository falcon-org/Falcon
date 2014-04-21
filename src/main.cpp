/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "options.h"
#include "daemon_instance.h"
#include "exceptions.h"

static void set_options(falcon::Options& opt) {
  /* *********************************************************************** */
  /* add command line options */
  opt.addCLIOption("daemon,d", "daemonize the build system");
  opt.addCLIOption("build,b", "launch a sequential build (default)");
  opt.addCLIOption("module,M",
                   po::value<std::string>(),
                   "use -M help for more info");
  opt.addCLIOption("config,f",
                   po::value<std::string>(),
                   "falcon configuration file");

  /* *********************************************************************** */
  /* add command line option and configuration file option (this options can be
   * setted on both configuration file or from the CLI) */
  opt.addCFileOption("graph,g",
                     po::value<std::string>()->default_value("makefile.json"),
                     "falcon graph file");
  opt.addCFileOption("port,p",
                     po::value<int>()->default_value(4242),
                     "the API listening port");
}

static int load_module(std::unique_ptr<falcon::Graph> g, std::string const& s) {

  if (0 == s.compare("dot")) {
    falcon::GraphGraphizPrinter ggp;
    g->accept(ggp);
  } else if (0 == s.compare("make")) {
    falcon::GraphMakefilePrinter gmp;
    g->accept(gmp);
  } else if (0 == s.compare("help")) {
    std::cout << "list of available modules: " << std::endl
      << "  dot    show the graph in DOT format" << std::endl
      << "  make   show the graph in Makefile format" << std::endl;
  } else {
    std::cerr << "module not supported: " << s << std::endl;
    return 1;
  }

  return 0;
}

int main (int const argc, char const* const* argv)
{
  falcon::Options opt;

  /* set the falcon's options */
  set_options(opt);

  /* parse the command line options */
  try {
    opt.parseOptions(argc, argv);
  } catch (falcon::Exception e) {
    if (e.getCode () != 0) { // the help throw a SUCCESS code, no error to show
      std::cerr << e.getErrorMessage() << std::endl;
    }
    return e.getCode();
  }

  std::unique_ptr<falcon::GlobalConfig> config(new falcon::GlobalConfig(opt));

  /* Analyze the graph given in the configuration file */
  falcon::GraphParser graphParser;
  try {
    graphParser.processFile(config->getJsonGraphFile());
  } catch (falcon::Exception e) {
    std::cerr << e.getErrorMessage () << std::endl;
    return e.getCode();
  }

  /* if a module has been requested to execute then load it and return */
  if (opt.isOptionSetted("module")) {
    return load_module(graphParser.getGraph(),
                       opt.vm_["module"].as<std::string>());
  }

  /* by default, falcon perform a sequential build.
   * user has to explicitely request to launch it using a daemon */
  falcon::DaemonInstance daemon(std::move(config));
  daemon.loadConf(graphParser.getGraph());
  daemon.start();

  return 0;
}
