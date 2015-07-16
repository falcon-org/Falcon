cmake_minimum_required(VERSION 2.8)
project(FALCON)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(thrift_USE_STATIC_LIBS "TRUE")
find_package(Thrift REQUIRED)

find_package(Gflags REQUIRED)
find_package(Glog REQUIRED)

set(Boost_USE_STATIC_LIBS "TRUE")
find_package( Boost "1.58" REQUIRED COMPONENTS program_options)

find_package( Libgit2 REQUIRED )

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
  gflags
  pthread)

add_executable(falcond
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_constants.cpp
  ${CMAKE_BINARY_DIR}/thrift/gen-cpp/FalconService_types.cpp
  src/util/event.cpp
  src/build_plan.cpp
  src/cache_fs.cpp
  src/cache_git_directory.cpp
  src/cache_manager.cpp
  src/command_server.cpp
  src/daemon_instance.cpp
  src/depfile.cpp
  src/depfile_parser.cpp
  src/fs.cpp
  src/graph.cpp
  src/graph_builder.cpp
  src/graph_consistency_checker.cpp
  src/graph_cycle.cpp
  src/graph_dependency_scan.cpp
  src/graph_hash.cpp
  src/graph_parallel_builder.cpp
  src/graph_printers.cpp
  src/graph_reloader.cpp
  src/graphparser.cpp
  src/json/json.c
  src/json/parser.cpp
  src/lazy_cache.cpp
  src/logging.cpp
  src/main.cpp
  src/options.cpp
  src/posix_subprocess.cpp
  src/posix_subprocess_manager.cpp
  src/stream_consumer.cpp
  src/stream_server.cpp
  src/watchman.cpp
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
  git2
  gflags
  )

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11 -ggdb3")

set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")


install(
  PROGRAMS
    ${CMAKE_BINARY_DIR}/falcond
    ${CMAKE_SOURCE_DIR}/clients/python/falcon
  DESTINATION bin)
install(
  FILES
    ${CMAKE_SOURCE_DIR}/clients/python/FalconService.py
    ${CMAKE_SOURCE_DIR}/clients/python/ttypes.py
    ${CMAKE_SOURCE_DIR}/clients/python/BuildOutputParser.py
  DESTINATION bin)
