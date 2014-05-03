# - Find GLOG
# Find the GLOG includes and library
# This module defines
#  glog_INCLUDE_DIR, where to find glog
#  glog_LIBRARIES, the libraries needed to use glog
#  also defined, but not for general use are
#  glog_LIBRARY, where to find the glog library.

IF(glog_USE_STATIC_LIBS)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(glog_USE_STATIC_LIBS)

find_path(glog_INCLUDE_DIR glog/logging.h NO_DEFAULT_PATH PATHS
  /opt/local/include
  /usr/local/include
  /usr/include
  /sw/include)

set(glog_NAMES ${glog_NAMES} glog)
find_library(glog_LIBRARY NAMES ${glog_NAMES} NO_DEFAULT_PATH PATHS
  /opt/local/lib
  /usr/local/lib
  /usr/lib
  /sw/lib)

if (glog_LIBRARY AND glog_INCLUDE_DIR)
  set(glog_LIBRARIES ${glog_LIBRARY})
  message(STATUS "Found glog: ${glog_LIBRARIES}")
else()
  message(FATAL_ERROR "Could not find glog library")
endif()
