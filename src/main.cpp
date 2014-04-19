/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <iostream>

#include "daemon_instance.h"
#include "exceptions.h"

int main (int const argc, char const* const* argv)
{
  /* TODO: better option management */
  if (argc != 2) {
    return 1;
  }

  falcon::DaemonInstance daemon;

  try {
    daemon.loadConf(argv[1]);
  }
  catch (falcon::Exception e) {
    std::cerr << e.getErrorMessage ();
    return 1;
  }

  daemon.start();

  return 0;
}
