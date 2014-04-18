/**
 * Copyright : falcon build system (c) 2014.
 * LICENSE : see accompanying LICENSE file for details.
 */

#include <graphbuilder.h>
#include <exceptions.h>
#include <iostream>

int main (int const argc, char const* const* argv)
{
  falcon::GraphBuilder graphBuilder;

  /* TODO: better option management */
  if (argc != 2) {
    return 1;
  }

  try
  {
    graphBuilder.processFile (argv[1]);
  }
  catch (falcon::Exception e)
  {
    std::cerr << e.getErrorMessage ();
  }

  return 0;
}
