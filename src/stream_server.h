/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef FALCON_STREAM_SERVER_H_
#define FALCON_STREAM_SERVER_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "stream_consumer.h"

namespace falcon {

/**
 * -- A server for streaming the build output to several clients.
 *
 * When a client connects to the streaming server, it will receive the output of
 * the last build that was started (it may be a build that already completed).
 * If there is no such build (ie we never built before), the client will wait
 * and receive the data of the next build to start.
 * A client will only receive the output of one build. When the output of the
 * build is sent completely, the client socket is closed.
 *
 * This class is designed in a way that guarantees that it is possible to start
 * a new build even if there are still slow client reading the output of a
 * previous build. Thus, we can have clients that read the output of an old
 * build and clients that read the output of the current build be connected at
 * the same time.
 *
 * -- Internal implementation:
 *
 * This class maintains a list of 'BuildInfo' structs corresponding to each
 * build that happened in reverse chronological order, ie the front of the list
 * is the last build. Each struct contains:
 * - A buffer containing the output of the build in json format;
 * - A refcount that counts the number of clients to which the buffer is
 *   currently being sent.
 * When a build completes and the refcount reaches 0, the struct is removed from
 * the list.
 *
 * This class maintains two lists:
 * - waiting_: this is the list of clients' file descriptors that are waiting
 *   for new data to come;
 * - fds_: this is the list of clients' file descriptors for which there is new
 *   data to be sent to. These file descriptors are monitored with poll.
 * The file descriptors are moved from one list to the other depending on its
 * state (waiting for new data vs ready to read new data).
 *
 * This class maintains a list of 'ClientInfo' structs corresponding to each
 * connected client. Each struct contains:
 * - an iterator to the corresponding file descriptor either in waiting_ or
 *   fds_ depending on the state of the client;
 * - An iterator to the BuildInfo struct corresponding to the build on which the
 *   client is reading the output;
 * - A pointer to indicate the amount of data that has been read in the internal
 *   buffer of the BuildInfo struct.
 *
 * -- Usage:
 *
 * // -- MAIN THREAD
 * // Create a streaming server listening on port 4343.
 * StreamServer server(4343);
 * // This blocks until the server is shutdown.
 * server.run();
 *
 * // -- WRITER THREAD(S)
 * // Write some data coming from stdout to the streaming server.
 * server.writeStdout(1, buf, len);
 */
class StreamServer : public IStreamConsumer {
 public:

  /**
   * Construct a stream server.
   */
  explicit StreamServer();

  virtual ~StreamServer();

  void openPort(unsigned int port);

  /**
   * Run the stream server. Will block indefinitely.
   */
  void run();

  /**
   * Stop the streaming server.
   */
  void stop();

  /**
   * Indicate that a new build has started.
   * Must be called after endBuild was called if it is not the first build.
   */
  void newBuild(unsigned int buildId);

  /**
   * Mark the current build as completed. Must be called after newBuild was
   * called.
   */
  void endBuild();

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
   * Move all the file descriptors in the waiting_ list to fds_ so that they are
   * monitored with poll.
   */
  void flushWaiting();

  /** Write a byte to eventFd_ so that poll is notified and returns.
   * This is used when we want poll to return because we updated the list of
   * file descriptors to monitor or because we are shutting down. */
  void notifyPoll();

  void writeBuf(const std::string& str);
  void writeBufEscapeJson(char* buf, size_t len);

  struct BuildInfo {
    unsigned int id;
    std::string buf;
    /* Refcount that counts the number of clients listening to the output stream
     * of this build. When it reaches 0 and the build completed, this structure
     * can be deallocated. */
    unsigned int refcount;
    bool buildCompleted;

    /* Used for displaying the trailing comma at the end of each json cmd. */
    bool firstChunk;

    BuildInfo(unsigned int i)
      : id(i), refcount(0), buildCompleted(false), firstChunk(true) {}
  };

  /** Remove a build from the list of builds. */
  void removeBuild(std::list<BuildInfo>::iterator it);

  /**
   * Helper function for writing a chunk of data comming from a command.
   * @param cmdId    id of the command;
   * @param buf      Buffer that contains the data;
   * @param len      Size of the buffer;
   * @param isStdout Define if the data comes from the standard output.
   */
  void writeCmdOutput(unsigned int cmdId, char* buf, size_t len, bool isStdout);

  /** Queue of builds. Each new build is pushed to the front. */
  std::list<BuildInfo> builds_;

  /* File descriptor of the server socket. */
  int serverSocket_;

  /* File descriptor used to notify poll. When data is appended to buf_, we
   * write to this fd in order to wake-up poll. */
  int eventFd_;

  /* List of fds monitored with ppoll. */
  std::list<int> fds_;

  /* List of fds for which buf_ has been entirely sent. They are put on hold in
   * this list and will be put back for monitoring each time new data
   * arrives. */
  std::list<int> waiting_;

  std::mutex mutex_;

  struct ClientInfo {
    /* Iterator to the BuildInfo structure corresponding to the build the client
     * is listening on. Equals to builds_.end() if there are no builds yet. */
    std::list<BuildInfo>::iterator itBuild;
    /* Current pointer in itBuild->buf. Everything before this pointer has been
     * already sent. */
    size_t bufPtr;
    /* Iterator to the fd entry in fds_ or waiting_, depending on isWaiting. */
    std::list<int>::iterator itFd;
    /* Indicate if the fd is waiting for new data. In this case, it is stored in
     * waiting_ and will be moved to fds_ when new data arrives. When a fd is
     * waiting for new data, bufPtr should be at the end of the buffer or
     * itBuild should be equal to builds_.end() (ie not assigned to a build
     * yet). */
    bool isWaiting;
  };

  /* Map a fd to some information about the client, such as the current position
   * in the buffer. */
  std::unordered_map<int, ClientInfo> map_;

  std::atomic_bool stopped_;
};

} // namespace falcon

#endif // FALCON_STREAM_SERVER_H_

