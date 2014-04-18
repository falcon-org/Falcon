project(FALCON)

add_executable(falcon
  src/main.cpp
  src/graph.cpp
  src/json/json.c
  src/json/parser.cpp)

include_directories(src)

# This project uses C++11
set (CMAKE_CXX_FLAGS "-Wall -std=c++11")

# Check the compiler type and set needed options
if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )

  execute_process (
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

  if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7) )
    message (FATAL_ERROR "${PRPOJECT_NAME} requires g++ 4.7 or greater. (actual is ${GCC_VERSION})")
  endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

else ()
  message (FATAL_ERROR "Your C++ compiler (${CMAKE_CXX_COMPILER_ID}) does not support C++11")
endif()
