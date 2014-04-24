/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "options.h"
#include "exceptions.h"
#include "logging.h"
#include <iostream>

namespace falcon {

Options::Options()
  : cliDesc_   ("Command Line options")
  , cFileDesc_ ("Configuration options")
  , desc_      ()
  , vm_ ()
{
  cliDesc_.add_options() ("help,h", "produce this help message");
}
Options::~Options() {}


void Options::addCLIOption(char const* name, char const* desc)
{
  this->cliDesc_.add_options() (name, desc);
}

void Options::addCLIOption(char const* name,
                           po::value_semantic const* vm,
                           char const* desc)
{
  this->cliDesc_.add_options() (name, vm, desc);
}

void Options::addCFileOption(char const* name, char const* desc)
{
  this->cFileDesc_.add_options() (name, desc);
}

void Options::addCFileOption(char const* name,
                             po::value_semantic const* vm,
                             char const* desc)
{
  this->cFileDesc_.add_options() (name, vm, desc);
}

void Options::parseOptions(int const argc, char const* const* argv)
{
  desc_.add(cliDesc_).add(cFileDesc_);
  try
  {
    po::store(po::parse_command_line(argc, argv, desc_), vm_);
    po::notify(vm_);
  } catch (std::exception& e) {
    THROW_ERROR(EINVAL, e.what());
  }

  /* If a configuration file is given has parameter, then load the options from
   * this given file, but load the CLI options again to ensure we keep the
   * options from the CLI (They have higher priority). */
  if (vm_.count("config")) {
    try {
      std::string configFileName = vm_["config"].as<std::string>();
      po::store(po::parse_config_file<char>(vm_["config"].as<std::string>().c_str(),
                                            cFileDesc_),
                vmFile_);

      // To ensure the CLI options get the most priority from the
      po::store(po::parse_command_line(argc, argv, desc_), vm_);
      po::notify(vm_);
    } catch (std::exception& e) {
      THROW_ERROR(EINVAL, e.what());
    }
  }

  if (vm_.count("help")) {
    std::cout << desc_ << std::endl;
    THROW_ERROR_CODE(0);
  }
}

bool Options::isOptionSetted(std::string const& opt) const {
  return vm_.count(opt);
}

GlobalConfig::GlobalConfig(Options const& opt) {
  setJsonGraphFile(opt.vm_["graph"].as<std::string>());
  setNetworkAPIPort(opt.vm_["api-port"].as<int>());
  setNetworkStreamPort(opt.vm_["stream-port"].as<int>());
  setWorkingDirectoryPath(opt.vm_["working-directory"].as<std::string>());

  /* Set the log system level */
  initlogging(opt.vm_["log-level"].as<falcon::Log::Level>());

  runSequentialBuild_ = opt.isOptionSetted("build");
  runDaemonBuilder_ = opt.isOptionSetted("daemon");
}

std::string const& GlobalConfig::getJsonGraphFile() const {
  return jsonGraphFile_;
}
void GlobalConfig::setJsonGraphFile(std::string const& f) {
  LOG(info) << "set json graph file: '" << f << "'";
  jsonGraphFile_ = f;
}
int GlobalConfig::getNetworkAPIPort() const { return networkAPIPort_; }
void GlobalConfig::setNetworkAPIPort(int const& p) {
  LOG(info) << "set network API port: '" << p << "'";
  networkAPIPort_ = p;
}
int GlobalConfig::getNetworkStreamPort() const { return networkStreamPort_; }
void GlobalConfig::setNetworkStreamPort(int const& p) {
  LOG(info) << "set network Stream port: '" << p << "'";
  networkStreamPort_ = p;
}
void GlobalConfig::setWorkingDirectoryPath(std::string const& s) {
  LOG(info) << "set working directory: '" << s << "'";
  workingDirectoryPath_ = s;
}
std::string const& GlobalConfig::getWorkingDirectoryPath() const {
  return workingDirectoryPath_;
}

bool GlobalConfig::runSequentialBuild() const { return runSequentialBuild_; }
bool GlobalConfig::runDaemonBuilder() const { return runDaemonBuilder_; }
}
