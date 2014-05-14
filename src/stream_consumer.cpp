/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "stream_consumer.h"

namespace falcon {

std::string BufferStreamConsumer::getStdout() const {
  return sscout_.str();
}
std::string BufferStreamConsumer::getStderr() const {
  return sscerr_.str();
}

void BufferStreamConsumer::writeStdout(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  sscout_.write(buf, len);
}

void BufferStreamConsumer::writeStderr(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  sscerr_.write(buf, len);
}

} // namespace falcon
