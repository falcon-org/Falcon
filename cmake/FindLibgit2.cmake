# - Find LibGit2
# Find the GLOG includes and library
# This module defines
#  libgit2_INCLUDE_DIR, where to find libgit2
#  libgit2_LIBRARIES, the libraries needed to use libgit2
#  also defined, but not for general use are
#  libgit2_LIBRARY, where to find the libgit2 library.

find_path(libgit2_INCLUDE_DIR git2.h NO_DEFAULT_PATH PATHS
  /opt/local/include
  /usr/local/include
  /usr/include
  /sw/include)

find_library(libgit2_LIBRARY NAMES libgit2.a libgit2.so libgit2.dylib NO_DEFAULT_PATH PATHS
  /opt/local/lib
  /usr/local/lib
  /usr/lib
  /sw/lib)

if (libgit2_LIBRARY AND libgit2_INCLUDE_DIR)
  set(libgit2_LIBRARIES ${libgit2_LIBRARY})
  message(STATUS "Found libgit2: ${libgit2_LIBRARIES}")
else()
  message(FATAL_ERROR "Could not find libgit2 library")
endif()
