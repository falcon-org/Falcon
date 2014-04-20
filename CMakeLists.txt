cmake_minimum_required(VERSION 2.8)

project(FALCON)

find_package( Boost REQUIRED COMPONENTS program_options )

add_executable(falcon
  src/main.cpp
  src/options.cpp
  src/graph.cpp
  src/json/json.c
  src/json/parser.cpp
  src/graphparser.cpp
  src/graphvisitors.cpp
  src/daemon_instance.cpp
  src/graph_sequential_builder.cpp
  src/posix_subprocess.cpp)

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11 -ggdb3")

include_directories(src ${Boost_INCLIDE_DIRS} )
target_link_libraries(falcon ${Boost_PROGRAM_OPTIONS_LIBRARY})
