cmake_minimum_required(VERSION 3.26)
project(graph2x)

set(CMAKE_CXX_STANDARD 20)

add_library(graph2x INTERFACE include/graph2x.hpp)
target_include_directories(graph2x INTERFACE include)

add_executable(example1 "examples/example1.cpp")
target_link_libraries(example1 PRIVATE graph2x)
