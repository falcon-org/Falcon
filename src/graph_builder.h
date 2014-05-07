/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_GRAPH_BUILDER_H_
#define FALCON_GRAPH_BUILDER_H_

#include <string>
#include <functional>

#include "graph.h"

namespace falcon {

enum class BuildResult { UNKNOWN, SUCCEEDED, INTERRUPTED, FAILED };
std::string toString(BuildResult v);

typedef std::function<void(BuildResult)> onBuildCompletedFn;

class IGraphBuilder {
 public:
  virtual ~IGraphBuilder() {}

  /**
   * Launch an asynchronous build.
   */
  virtual void startBuild() = 0;

  /**
   * Interrupt the build. The callback will be called with the INTERRUPTED
   * code.
   */
  virtual void interrupt() = 0;

  /**
   * Wait until the current build completes.
   */
  virtual void wait() = 0;

  virtual BuildResult getResult() const = 0;
};

} // namespace falcon

#endif // FALCON_GRAPH_BUILDER_H_
