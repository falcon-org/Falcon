/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "stream_consumer.h"

namespace falcon {

void StdoutStreamConsumer::newCommand(unsigned int cmdId,
                                      const std::string& cmd) {
  std::cout << cmd << std::endl;
}

void StdoutStreamConsumer::endCommand(unsigned int cmdId,
                                      SubProcessExitStatus status) {}

void StdoutStreamConsumer::writeStdout(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  std::cout.write(buf, len);
}

void StdoutStreamConsumer::writeStderr(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  std::cerr.write(buf, len);
}

void StringStreamConsumer::newCommand(unsigned int cmdId,
                                      const std::string& cmd) { }

void StringStreamConsumer::endCommand(unsigned int cmdId,
                                      SubProcessExitStatus status) {}

std::string StringStreamConsumer::getCoutString() const {
  return sscout_.str();
}
std::string StringStreamConsumer::getCerrString() const {
  return sscerr_.str();
}

void StringStreamConsumer::writeStdout(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  sscout_.write(buf, len);
}

void StringStreamConsumer::writeStderr(unsigned int cmdId,
                                       char* buf, std::size_t len) {
  sscerr_.write(buf, len);
}


} // namespace falcon
