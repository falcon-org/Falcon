/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_STREAM_CONSUMER_H_
#define FALCON_STREAM_CONSUMER_H_

#include <cstring>
#include <sstream>

namespace falcon {

enum class SubProcessExitStatus;

/**
 * Interface required by PosixSubProcess.
 */
class IStreamConsumer {
 public:
  virtual ~IStreamConsumer() {}
  virtual void writeStdout(unsigned int cmdId, char* buf, std::size_t len) = 0;
  virtual void writeStderr(unsigned int cmdId, char* buf, std::size_t len) = 0;
};

/**
 * A stream consumer that stores stdout and stderr in a buffer.
 */
class BufferStreamConsumer : public IStreamConsumer {
public:
  virtual ~BufferStreamConsumer() {}
  virtual void writeStdout(unsigned int cmdId, char* buf, std::size_t len);
  virtual void writeStderr(unsigned int cmdId, char* buf, std::size_t len);

  std::string getStdout() const;
  std::string getStderr() const;
private:
  std::stringstream sscout_;
  std::stringstream sscerr_;
};

} // namespace falcon

#endif // FALCON_STREAM_CONSUMER_H_
