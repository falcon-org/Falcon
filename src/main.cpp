/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>
#include <cstdlib>

#include "options.h"
#include "daemon_instance.h"
#include "exceptions.h"
#include "logging.h"

static void set_options(falcon::Options& opt) {
  /* get the default working directory path from then environment variable */
  char const* pwd = NULL;
  pwd = getenv("PWD"); /* No need to free this string */
  if (!pwd) {
    LOG(fatal) << "enable to get the PWD";
    return;
  }
  std::string pwdString(pwd);

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
  opt.addCFileOption("working-directory",
                     po::value<std::string>()->default_value(pwdString),
                     "falcon working directory path");
  opt.addCFileOption("graph",
                     po::value<std::string>()->default_value("makefile.json"),
                     "falcon graph file");
  opt.addCFileOption("api-port",
                     po::value<int>()->default_value(4242),
                     "the API listening port");
  opt.addCFileOption("stream-port",
                     po::value<int>()->default_value(4343),
                     "stream port");
  opt.addCFileOption("log-level",
                     po::value<falcon::Log::Level>()->default_value(
                       falcon::Log::Level::error),
                     "define the log level");
}

static int load_module(std::unique_ptr<falcon::Graph> g, std::string const& s) {

  LOG(info) << "load module '" << s << "'";

  if (0 == s.compare("dot")) {
    falcon::GraphGraphizPrinter ggp(std::cout);
    g->accept(ggp);
  } else if (0 == s.compare("make")) {
    falcon::GraphMakefilePrinter gmp(std::cout);
    g->accept(gmp);
  } else if (0 == s.compare("help")) {
    std::cout << "list of available modules: " << std::endl
      << "  dot    show the graph in DOT format" << std::endl
      << "  make   show the graph in Makefile format" << std::endl;
  } else {
    LOG(error) << "module '" << s << "' not supported";
    return 1;
  }

  return 0;
}

int main (int const argc, char const* const* argv)
{
  falcon::Options opt;
  falcon::initlogging(falcon::Log::Level::error);

  set_options(opt);

  /* parse the command line options */
  try {
    opt.parseOptions(argc, argv);
  } catch (falcon::Exception e) {
    if (e.getCode () != 0) { // the help throw a SUCCESS code, no error to show
      LOG(error) << e.getErrorMessage();
    }
    return e.getCode();
  }

  std::unique_ptr<falcon::GlobalConfig> config(new falcon::GlobalConfig(opt));

  /* Analyze the graph given in the configuration file */
  falcon::GraphParser graphParser;
  try {
    graphParser.processFile(config->getJsonGraphFile());
  } catch (falcon::Exception e) {
    LOG(error) << e.getErrorMessage ();
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
