/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include "logging.h"

namespace falcon {

void initlogging(Log::Level const& e) {
  logging::core::get()->set_filter
    ( logging::trivial::severity >= e );
}

}
