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
  this->setJsonGraphFile(opt.vm_["graph"].as<std::string>());
  this->setNetworkAPIPort(opt.vm_["port"].as<int>());

  /* Set the log system level */
  initlogging(opt.vm_["log-level"].as<falcon::Log::Level>());

  if (opt.isOptionSetted("build")) {
    runSequentialBuild_ = true;
  }

  if (opt.isOptionSetted("daemon")) {
    runDaemonBuilder_ = true;
  }

  workingDirectoryPath_ = opt.vm_["working-directory"].as<std::string>();
}

std::string const& GlobalConfig::getJsonGraphFile() const {
  return jsonGraphFile_;
}
void GlobalConfig::setJsonGraphFile(std::string const& f) {
  LOG(info) << "set json graph file: " << f << "'";
  jsonGraphFile_ = f;
}
int const& GlobalConfig::getNetworkAPIPort() const { return networkAPIPort_; }
void GlobalConfig::setNetworkAPIPort(int const& p) {
  LOG(info) << "set network API port: " << p << "'";
  networkAPIPort_ = p;
}

bool GlobalConfig::runSequentialBuild() const { return runSequentialBuild_; }
bool GlobalConfig::runDaemonBuilder() const { return runDaemonBuilder_; }
std::string const& GlobalConfig::getWorkingDirectoryPath() const {
  return workingDirectoryPath_;
}

}
