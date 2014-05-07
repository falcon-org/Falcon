/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "graph_builder.h"
#include "exceptions.h"

namespace falcon {

std::string toString(BuildResult v) {
  switch (v) {
    case BuildResult::UNKNOWN: return "UNKNOWN";
    case BuildResult::SUCCEEDED: return "SUCCEEDED";
    case BuildResult::INTERRUPTED: return "INTERRUPTED";
    case BuildResult::FAILED: return "FAILED";
    default: THROW_ERROR(EINVAL, "Unrecognized BuildResult");
  }
}

} // namespace falcon
