# - Find thrift
# Find the thrift includes and library
# This module defines
#  thrift_INCLUDE_DIR, where to find thrift
#  thrift_LIBRARIES, the libraries needed to use thrift.
#  also defined, but not for general use are
#  thrift_LIBRARY, where to find the thrift library.

IF(thrift_USE_STATIC_LIBS)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(thrift_USE_STATIC_LIBS)

find_path(thrift_INCLUDE_DIR thrift/Thrift.h NO_DEFAULT_PATH PATHS
  /opt/local/include
  /usr/local/include
  /usr/include
  /sw/include)

set(thrift_NAMES ${thrift_NAMES} thrift)
find_library(thrift_LIBRARY NAMES ${thrift_NAMES} NO_DEFAULT_PATH PATHS
  /opt/local/lib
  /usr/local/lib
  /usr/lib
  /sw/lib)

if (thrift_LIBRARY AND thrift_INCLUDE_DIR)
  set(thrift_LIBRARIES ${thrift_LIBRARY})
  message(STATUS "Found thrift: ${thrift_LIBRARIES}")
else()
  message(FATAL_ERROR "Could not find thrift library")
endif()

find_program(thrift_COMPILER thrift DOC "Thrift compiler")

if (thrift_COMPILER)
  message(STATUS "Found thrift compiler: ${thrift_COMPILER}")
else()
  message(FATAL_ERROR "Could not find thrift compiler")
endif()
