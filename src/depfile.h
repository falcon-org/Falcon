/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_DEPFILE_H_
# define FALCON_DEPFILE_H_

#include "graph.h"

namespace falcon {

class WatchmanClient;

/**
 * This file contains the routines for updating a graph with new dependencies to
 * a rule given the content of a depfile. It is supposed that the depfile of a
 * rule applies only for its first output.
 */
class Depfile {
  public:

  enum class Res { SUCCESS, ERROR_CANNOT_OPEN, ERROR_INVALID_FILE };

  /**
   * Update the implicit dependencies of a rule with the dependencies defined by
   * a buffer containing dependencies in Makfile format.
   *
   * @param buf            Buffer that contains the implicit dependencies in
   *                       Makefile format.
   * @param rule           Rule to be updated with the new implicit
   *                       dependencies.
   * @param watchmanClient Watchman client.
   * @param graph          Graph that contains the rule.
   *
   * @return        True on success, or false on error (invalid format).
   */
  static bool load(std::string& buf, Rule* rule,
                   WatchmanClient* watchmanClient, Graph& graph);

  /**
   * Update the implicit dependencies of a rule with the dependencies defined by
   * a depfile.
   *
   * @param depfile        Depfile that contains the dependencies, in Makefile
   *                       format.
   * @param rule           Rule to be updated with the new implicit
   *                       dependencies.
   * @param watchmanClient Watchman client.
   * @param graph          Graph that contains the rule.
   *
   * @return        error code indicating a possible error.
   */
  static Res loadFromfile(const std::string& depfile, Rule *rule,
                          WatchmanClient* watchmanClient, Graph& graph);

 private:

  /**
   * Set a new dependency for the given rule, creating a new target if needed.
   * If the target is already a dependency, this does nothing and returns true.
   *
   * @param dep            Path of the dependency.
   * @param rule           Rule to be updated with the new input.
   * @param watchmanClient Watchman client.
   * @param graph          Graph that contains the rule.
   */
  static void setRuleDependency(const std::string& dep, Rule* rule,
                                WatchmanClient* watchmanClient, Graph& graph);
};

} // namespace falcon

#endif // FALCON_DEPFILE_H_
