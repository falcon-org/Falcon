/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_STREAM_CONSUMER_H_
#define FALCON_STREAM_CONSUMER_H_

#include <cstring>
#include <sstream>

namespace falcon {

class IStreamConsumer {
 public:
  virtual ~IStreamConsumer() {}
  virtual void writeStdout(unsigned int cmdId, char* buf, size_t len) = 0;
  virtual void writeStderr(unsigned int cmdId, char* buf, size_t len) = 0;
};

/**
 * A stream consumer that outputs everything to stdout.
 */
class StdoutStreamConsumer : public IStreamConsumer {
 public:
  virtual ~StdoutStreamConsumer() {}
  virtual void writeStdout(unsigned int cmdId, char* buf, size_t len);
  virtual void writeStderr(unsigned int cmdId, char* buf, size_t len);
};

class StringStreamConsumer : public IStreamConsumer {
public:
  virtual ~StringStreamConsumer() {}
  virtual void writeStdout(unsigned int cmdId, char* buf, size_t len);
  virtual void writeStderr(unsigned int cmdId, char* buf, size_t len);

  std::string getCoutString() const;
  std::string getCerrString() const;
private:
  std::stringstream sscout_;
  std::stringstream sscerr_;
};

} // namespace falcon

#endif // FALCON_STREAM_CONSUMER_H_
