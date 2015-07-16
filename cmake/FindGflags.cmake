# - Find GFlags
# Find the GFlags includes and library
# This module defines
#  gflags_INCLUDE_DIR, where to find gflags
#  gflags_LIBRARIES, the libraries needed to use gflags
#  also defined, but not for general use are
#  gflags_LIBRARY, where to find the gflags library.

set(gflags_NAMES ${gflags_NAMES} gflags)
find_library(gflags_LIBRARY NAMES ${gflags_NAMES} NO_DEFAULT_PATH PATHS
  /opt/local/lib
  /usr/local/lib
  /usr/lib
  /sw/lib)

if (gflags_LIBRARY)
  set(gflags_LIBRARIES ${gflags_LIBRARY})
  message(STATUS "Found gflags: ${gflags_LIBRARIES}")
else()
  message(FATAL_ERROR "Could not find gflags library")
endif()
