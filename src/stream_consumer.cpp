/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "stream_consumer.h"

namespace falcon {

void StdoutStreamConsumer::writeStdout(unsigned int cmdId,
                                       char* buf, size_t len) {
  std::cout.write(buf, len);
}

void StdoutStreamConsumer::writeStderr(unsigned int cmdId,
                                       char* buf, size_t len) {
  std::cout.write(buf, len);
}

} // namespace falcon
