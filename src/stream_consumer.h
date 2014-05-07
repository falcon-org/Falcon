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

class IStreamConsumer {
 public:
  virtual ~IStreamConsumer() {}
  virtual void newCommand(unsigned int cmdId, const std::string& cmd) = 0;
  virtual void endCommand(unsigned int cmdId, SubProcessExitStatus status) = 0;
  virtual void writeStdout(unsigned int cmdId, char* buf, std::size_t len) = 0;
  virtual void writeStderr(unsigned int cmdId, char* buf, std::size_t len) = 0;
  virtual void cacheRetrieveAction(const std::string& path) = 0;
};

/**
 * A stream consumer that outputs everything to stdout / stderr.
 */
class StdoutStreamConsumer : public IStreamConsumer {
 public:
  virtual ~StdoutStreamConsumer() {}
  virtual void newCommand(unsigned int cmdId, const std::string& cmd);
  virtual void endCommand(unsigned int cmdId, SubProcessExitStatus status);
  virtual void writeStdout(unsigned int cmdId, char* buf, std::size_t len);
  virtual void writeStderr(unsigned int cmdId, char* buf, std::size_t len);
  virtual void cacheRetrieveAction(const std::string& path);
};

/**
 * A stream consumer that stores stdout and stderr in a buffer.
 */
class StringStreamConsumer : public IStreamConsumer {
public:
  virtual ~StringStreamConsumer() {}
  virtual void newCommand(unsigned int cmdId, const std::string& cmd);
  virtual void endCommand(unsigned int cmdId, SubProcessExitStatus status);
  virtual void writeStdout(unsigned int cmdId, char* buf, std::size_t len);
  virtual void writeStderr(unsigned int cmdId, char* buf, std::size_t len);
  virtual void cacheRetrieveAction(const std::string& path);

  std::string getCoutString() const;
  std::string getCerrString() const;
private:
  std::stringstream sscout_;
  std::stringstream sscerr_;
};

} // namespace falcon

#endif // FALCON_STREAM_CONSUMER_H_
