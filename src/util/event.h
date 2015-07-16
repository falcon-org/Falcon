/**
 * Copyright : falcon build system (c).
 * LICENSE : see accompanying LICENSE file for details.
 */

#ifndef EVENT_H_
# define EVENT_H_

namespace falcon {

/**
 * @class EventNotifier
 * @brief event class management
 *
 * This class provides a simple object for raising events.
 *
 * The aim is to hide the implementation of the event objects (pipe,
 * eventfd...) and to make the use of events as simple as possible.
 */
class EventNotifier {
  public:
    /**
     * Construct a new event notifier object
     */
    EventNotifier();
    ~EventNotifier();

    /**
     * raise an event
     */
    int raise();

    /**
     * flush the event
     */
    void flush();

    /**
     * get a file descriptor to listen on it.
     */
    int get() { return rfd_; }

  private:
    int rfd_;
    int wfd_;
};

}

#endif /* ! EVENT_H_  */
