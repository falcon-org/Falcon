/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "logging.h"
#include "options.h"

#include <cstdlib>

namespace falcon {

void defaultlogging(std::string const& programName,
                    google::LogSeverity lvl,
                    std::string const& logDir) {
  if (logDir.empty()) {
    /* Request GLOG to log everything on stderr */
    FLAGS_logtostderr = 1;
  } else {
    /* If GLOG_log_dir has been specified, at least, we would like to see ERROR
     * and FATAL logs to stderr: */
    FLAGS_stderrthreshold = lvl;
    FLAGS_log_dir = logDir.c_str();
  }

  FLAGS_minloglevel = lvl;
  FLAGS_v = -1; /* We don't want verbose message */

  google::InitGoogleLogging(programName.c_str());
}

void testlogging(std::string const& programName) {
  FLAGS_logtostderr = 1;

  FLAGS_v = -1; /* We don't want verbose message */

  FLAGS_minloglevel = google::WARNING;
  google::InitGoogleLogging(programName.c_str());
}

}
