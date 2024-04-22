cmake_minimum_required(VERSION 3.26)
project(graph2x)

set(CMAKE_CXX_STANDARD 20)

add_library(graph2x INTERFACE include/graph2x.hpp)
target_include_directories(graph2x INTERFACE include)

option(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES "Build example programs with graphics that are dependent on axxegro" ON)

add_executable(example1 "examples/example1.cpp")
target_link_libraries(example1 PRIVATE graph2x)


if(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES)
	add_subdirectory("axxegro")
	add_executable(graph_editor "examples/graph_editor.cpp")
	target_link_libraries(graph_editor PRIVATE graph2x axxegro)
endif ()