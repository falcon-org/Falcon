project(FALCON)

add_executable(falcon
  src/main.cpp
  src/graph.cpp
  src/json/json.c
  src/json/parser.cpp
  src/graphparser.cpp
  src/graphvisitors.cpp
  src/daemon_instance.cpp)

include_directories(src)

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11 -ggdb3")
