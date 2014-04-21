/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_LOGGING_H_
# define FALCON_LOGGING_H_

# define BOOST_LOG_DYN_LINK

# include <boost/log/core.hpp>
# include <boost/log/trivial.hpp>
# include <boost/log/expressions.hpp>

# define LOG BOOST_LOG_TRIVIAL

/* Use logging to shortcut the boost::log namespace */
namespace logging = boost::log;

namespace falcon {

class Log {
public:
  typedef logging::trivial::severity_level Level;
};
void initlogging(Log::Level const& l);

}

#endif /* !FALCON_LOGGING_H_ */
