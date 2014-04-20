/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "options.h"
#include "exceptions.h"
#include <iostream>

namespace falcon {

Options::Options()
  : desc_ ("Generic OptionsParser")
  , vm_ ()
{
  desc_.add_options() ("help,h", "produce this help message");
}
Options::~Options() {}


void Options::addOption(char const* name, char const* desc)
{
  this->desc_.add_options() (name, desc);
}

void Options::addOption(char const* name,
                        po::value_semantic const* vm,
                        char const* desc)
{
  this->desc_.add_options() (name, vm, desc);
}

void Options::parseOptions(int const argc, char const* const* argv)
{
  try
  {
    po::store(po::parse_command_line(argc, argv, desc_), vm_);
    po::notify(vm_);
  } catch (std::exception& e) {
    THROW_ERROR(EINVAL, e.what());
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
