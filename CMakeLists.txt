cmake_minimum_required(VERSION 2.8)
project(FALCON)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(thrift_USE_STATIC_LIBS "TRUE")
set(Boost_USE_STATIC_LIBS OFF)

find_package( Boost "1.55" REQUIRED COMPONENTS program_options log)
find_package(Thrift REQUIRED)

include (cmake/GenThrift.cmake)
include (cmake/GenClients.cmake)

buildThriftCpp(thrift/FalconService.thrift)
genPythonClient()

add_executable(falcon
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_types.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_constants.cpp
  src/main.cpp
  src/options.cpp
  src/logging.cpp
  src/graph.cpp
  src/json/json.c
  src/json/parser.cpp
  src/graphparser.cpp
  src/graphvisitors.cpp
  src/daemon_instance.cpp
  src/graph_sequential_builder.cpp
  src/posix_subprocess.cpp
  src/server.cpp
  )

include_directories(src
  ${thrift_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/
  ${Boost_INCLIDE_DIRS})

target_link_libraries(falcon
  ${thrift_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_LOG_LIBRARY}
  pthread)

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11 -ggdb3")
