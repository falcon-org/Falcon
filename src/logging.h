/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_LOGGING_H_
# define FALCON_LOGGING_H_

# include <glog/logging.h>

/* We don't want to use Verbose log message */
# undef VLOG

/*
 * GLOG: Google Logging system:
 * see: http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 */

namespace falcon {

/*!
 * @function defaultlogging:
 * @brief configure the Google logging system
 *
 * @param programName: the name to show (you may want the program name)
 * @param lvl: the minimum bound of log level to show
 * @param logDir: a directory where Google GLOG will write log files. If the
 *                given string then the logs will be only print the stderr */
void defaultlogging(std::string const& programName,
                    google::LogSeverity lvl,
                    std::string const& logDir);

/*! Configure the log system */
void testlogging(std::string const& programName);

}

#endif /* !FALCON_LOGGING_H_ */
