/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "options.h"
#include "exceptions.h"
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

  /* This part will be enable in future when we will use a configuration file
   * TODO: see main.cpp to enable the --config|-f option */
#if 0
  try
  {
    std::string configFileName = vm_["config"].as<std::string>();
    po::store(po::parse_config_file<char>(vm_["config"].as<std::string>().c_str(),
                                          cFileDesc_),
              vm_);
    po::notify(vm_);
  } catch (std::exception& e) {
    THROW_ERROR(EINVAL, e.what());
  }
#endif

  if (vm_.count("help")) {
    std::cout << desc_ << std::endl;
    THROW_ERROR_CODE(0);
  }
}

bool Options::isOptionSetted(std::string const& opt) const {
  return vm_.count(opt);
}

GlobalConfig::GlobalConfig(Options const& opt) {
  jsonGraphFile_ = opt.vm_["graph"].as<std::string>();

  if (!opt.isOptionSetted("daemon") || opt.isOptionSetted("build")) {
    runSequentialBuild_ = true;
    runDaemonBuilder_ = false;
  } else {
    runDaemonBuilder_ = true;
    runSequentialBuild_ = false;
  }
}

std::string const& GlobalConfig::getJsonGraphFile() const {
  return jsonGraphFile_;
}
void GlobalConfig::setJsonGraphFile(std::string const& f) {
  jsonGraphFile_ = f;
}

bool GlobalConfig::runSequentialBuild() const {
  return runSequentialBuild_;
}
bool GlobalConfig::runDaemonBuilder() const {
  return runDaemonBuilder_;
}

}
