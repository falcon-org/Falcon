/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_STREAM_SERVER_H_
#define FALCON_STREAM_SERVER_H_

#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "stream_consumer.h"

namespace falcon {

/**
 * Implement a server for streaming the build output to several clients.
 * This class maintains an internal buffer that contains all the data written
 * since its creation. When a new client connects to the server, the whole
 * buffer will be written to the client and then the client will be put in a
 * pool of "waiting" clients. (See waiting_).
 *
 * As new data arrives, clients are moved from the pool of waiting clients to
 * the pool of reading clients. (See fds_). The file descriptors of those
 * clients are monitored with poll.
 *
 * For each client, we maintain a pointer in the internal buffer (buf_) so that
 * we keep track of what has been sent to each client.
 *
 * TODO:
 * - Implement a shutdown mechanism. Right now this runs forever;
 * - When a build completes, clear the internal buffer. We need to wait for
 *   every clients that were reading it to complete before releasing the buffer.
 *   In order to deal with that, we can maintain a doubly-linked of buffers -
 *   one per build - and have a refcounts of the number of clients reading from
 *   each of them. When a buffer has a refcount of zero, it can be free'd.
 *
 * Usage:
 *
 * // -- MAIN THREAD
 * // Create a streaming server listening on port 4343.
 * StreamServer server(4343);
 * // This blocks until the server is shutdown.
 * server.run();
 *
 * // -- WRITER THREAD(S)
 * // Write some data to the streaming server.
 * server.writeStdout(1, buf, len);
 */
class StreamServer : public IStreamConsumer {
 public:

  /**
   * Construct a stream server.
   * @param port Port on which to listen for connections.
   */
  explicit StreamServer(unsigned int port);

  virtual ~StreamServer();

  /**
   * Run the stream server. Will block indefinitely.
   */
  void run();

  /**
   * Write a buffer coming from the standard output of a command.
   * @param cmdId Id of the command.
   * @param buf   Buffer to be written.
   * @param len   Size of the buffer.
   */
  void writeStdout(unsigned int cmdId, char* buf, size_t len);

  /**
   * Write a buffer coming from error output of a command.
   * @param cmdId Id of the command.
   * @param buf   Buffer to be written.
   * @param len   Size of the buffer.
   */
  void writeStderr(unsigned int cmdId, char* buf, size_t len);

 private:

  /** This will call poll to process events. */
  void processEvents();


  /** Read 8 bytes from eventFd_. */
  void flushEventFd();

  /**
   * Called when serverSocket_ is ready to accept new clients. This will accept
   * all the clients ready to be accepted.
   */
  void acceptClients();

  /**
   * Create a new client from a file descriptor.
   * @param fd File descriptor of the client socket.
   */
  void createClient(int fd);

  /**
   * Called when a socket is ready for a read.
   * @param fd Fd of the socket ready to be read.
   */
  void processClient(int fd);

  /**
   * Close the given client. This closes the socket and updates fds_ and map_.
   */
  void closeClient(int fd);

  /**
   * Write data to all clients.
   * @param buf Buffer to write.
   * @param len Size of the buffer.
   */
  void writeBuf(char* buf, size_t len);

  /**
   * Remove a file descriptor from the list of file descriptor to be monitored
   * with poll.
   * @praram fdPos Position of the file descriptor in fds_.
   */
  void removeFd(size_t fdPos);

  /* File descriptor of the server socket. */
  int serverSocket_;

  /* File descriptor used to notify poll. When data is appended to buf_, we
   * write to this fd in order to wake-up poll. */
  int eventFd_;

  /* List of fds monitored with ppoll. Contains serverSocket_ and eventFd_. */
  std::vector<pollfd> fds_;

  /* List of fds for which buf_ has been entirely sent. They are put on hold in
   * this list and will be put back for monitoring each time new data
   * arrives. */
  std::vector<int> waiting_;

  /* Buffer that contains the build output. */
  std::string buf_;

  /* Mutex to protect accesses to buf_, map_, and waiting_. */
  std::mutex mutex_;

  struct ClientInfo {
    /* Current pointer in buf_. Everything before this pointer has been already
     * sent. */
    size_t bufPtr;
    /* Position of the fd in fds_ or waiting_, depending on isWaiting. */
    size_t fdPos;
    /* Indicate if the fd is waiting for new data. In this case, it is stored in
     * waiting_ and will be moved to fds_ when new data arrives. When a fd is
     * waiting for new data, bufPtr should be at the end of the buffer. */
    bool isWaiting;
  };

  /* Map a fd to some information about the client, such as the current position
   * in the buffer. */
  std::unordered_map<int, ClientInfo> map_;
};

} // namespace falcon

#endif // FALCON_STREAM_SERVER_H_

