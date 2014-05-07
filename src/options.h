/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_OPTIONS_H_
# define FALCON_OPTIONS_H_

# include <boost/program_options.hpp>
# include "logging.h"

/*!
 * @namespace po
 * a short cut for boost::program_options */
namespace po = boost::program_options;

namespace falcon {

class Options {
public:
  Options();
  ~Options();

  /*! @brief addCLIOption
   *  it adds an option for the command line in the boost::program_options
   *  description. This option doesn't expect an argument.
   *
   *  @param opt is the option string "long,s" where long
   *             is a long option (--long) and s is the associated shord
   *             option (-s).
   *  @param desc is the option description */
  void addCLIOption (char const* opt, char const* desc);
  /*! @brief addCLIOption
   *  it does the same but with an argument required.
   *
   *  @param opt is the option string "long,s" where long
   *             is a long option (--long) and s is the associated shord
   *             option (-s).
   *  @param arg defines the type of the required argument.
   *             See boost::program_option for more
   *             information.
   *  @param desc is the option's description */
  void addCLIOption (char const* opt, po::value_semantic const* arg,
                     char const* desc);

  /*!
   * Same than addCLIOption but this option can also be read from a
   * configuration file */
  void addCFileOption (char const* opt, char const* desc);
  /*!
   * Same than addCLIOption but this option can also be read from a
   * configuration file */
  void addCFileOption (char const* opt, po::value_semantic const* arg,
                       char const* desc);

  /*! @brief parseOptions
   *  it parses all of the registered options from the registered
   *  module (see addOption).
   *
   *  @param argc is the number of argument given in command
   *              line option.
   *  @param argv is the argument array givent in
   *              command line option. */
  void parseOptions (int const argc, char const* const* argv);

  bool isOptionSetted(std::string const& opt) const;
private:
  std::string programName_;
public:
  std::string const& getProgramName() const;

private:
  std::string logDirectory_;
public:
  std::string const& getLogDirectory() const;

private:
  /* Command line options description */
  po::options_description cliDesc_;
  /* Option that will be available from file and from CLI */
  po::options_description cFileDesc_;
  po::options_description desc_;
public:
  po::variables_map vm_;/*!< see bosst::program_option */
  po::variables_map vmFile_;/*!< see bosst::program_option */
};

class GlobalConfig {
public:
  GlobalConfig(Options const& opt);

  /* *********************************************************************** */
  /* Option that can be changed at run-time */
private:
  std::string jsonGraphFile_;
public:
  std::string const& getJsonGraphFile() const;
  void setJsonGraphFile(std::string const& f);

private:
  int networkAPIPort_;
public:
  int getNetworkAPIPort() const;
  void setNetworkAPIPort(int const& p);

private:
  int networkStreamPort_;
public:
  int getNetworkStreamPort() const;
  void setNetworkStreamPort(int const& p);

private:
  std::string workingDirectoryPath_;
public:
  std::string const& getWorkingDirectoryPath() const;
  void setWorkingDirectoryPath(std::string const&);

  /* *********************************************************************** */
  /* Option that will need to restart the falcon process */
private:
  std::string programName_;
public:
  std::string const& getProgramName() const;

private:
  std::string logDirectory_;
public:
  std::string const& getLogDirectory() const;

private:
  std::string falconDir_;
public:
  std::string const& getFalconDir() const;

private:
  bool runSequentialBuild_;
public:
  bool runSequentialBuild() const;

private:
  bool runDaemonBuilder_;
public:
  bool runDaemonBuilder() const;
};

}

#endif /* !FALCON_OPTIONS_H_ */
