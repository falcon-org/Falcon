/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_OPTIONS_H_
# define FALCON_OPTIONS_H_

# include <boost/program_options.hpp>

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
  po::options_description cliDesc_;/*!< see bosst::program_option */
  po::options_description cFileDesc_;/*!< see bosst::program_option */
  po::options_description desc_;
public:
  po::variables_map vm_;/*!< see bosst::program_option */
};

class GlobalConfig {
public:
  GlobalConfig(Options const& opt);

  /* *********************************************************************** */
  /* Option that can be changed at run-time */
private: /* variables */
  std::string jsonGraphFile_;
public: /* Accessors */
  std::string const& getJsonGraphFile() const;
  void setJsonGraphFile(std::string const& f);

  /* *********************************************************************** */
  /* Option that will need to restart the falcon process */
private: /* variables */
  bool runSequentialBuild_;
  bool runDaemonBuilder_;
public: /* Accessors */
  bool runSequentialBuild() const;
  bool runDaemonBuilder() const;

};

}

#endif /* !FALCON_OPTIONS_H_ */
