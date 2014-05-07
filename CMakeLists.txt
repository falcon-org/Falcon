cmake_minimum_required(VERSION 2.8)
project(FALCON)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(thrift_USE_STATIC_LIBS "TRUE")
find_package(Thrift REQUIRED)

set(glog_USE_STATIC_LIBS "TRUE")
find_package(Glog REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost "1.55" REQUIRED COMPONENTS program_options)

include (cmake/GenThrift.cmake)
include (cmake/GenClients.cmake)

buildThriftCpp(thrift/FalconService.thrift)
genPythonClient()

add_executable(tests/jsonparser
  src/test.cpp
  src/json/json.c
  src/json/parser.cpp
  src/json/tests/parser.cpp)

add_executable(tests/exceptions
  src/test.cpp
  src/tests/exceptions.cpp)

add_executable(tests/posix_subprocess
  src/options.cpp
  src/logging.cpp
  src/test.cpp
  src/stream_consumer.cpp
  src/posix_subprocess.cpp
  src/tests/posix_subprocess.cpp)
target_link_libraries(tests/posix_subprocess
  ${thrift_LIBRARIES}
  ${glog_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  pthread)

add_executable(falcond
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_types.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_constants.cpp
  src/daemon_instance.cpp
  src/graph.cpp
  src/graph_hash.cpp
  src/graph_cycle.cpp
  src/graph_dependency_scan.cpp
  src/graph_printers.cpp
  src/graphparser.cpp
  src/graph_builder.cpp
  src/json/json.c
  src/json/parser.cpp
  src/logging.cpp
  src/main.cpp
  src/options.cpp
  src/posix_subprocess.cpp
  src/posix_subprocess_manager.cpp
  src/command_server.cpp
  src/stream_consumer.cpp
  src/stream_server.cpp
  src/watchman.cpp
  src/depfile.cpp
  src/depfile_parser.cpp
  src/graph_consistency_checker.cpp
  src/build_plan.cpp
  src/graph_parallel_builder.cpp
  src/cache_manager.cpp
  src/fs.cpp
  )

include_directories(src
  ${thrift_INCLUDE_DIR}
  ${glog_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/
  ${Boost_INCLIDE_DIRS})

target_link_libraries(falcond
  ${thrift_LIBRARIES}
  ${glog_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  pthread
  crypto
  )

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11 -ggdb3")

set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
