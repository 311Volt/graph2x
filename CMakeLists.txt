cmake_minimum_required(VERSION 3.26)
project(graph2x)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE GRAPH2X_HEADERS "include/*.hpp")

add_library(graph2x INTERFACE ${GRAPH2X_HEADERS})
target_include_directories(graph2x INTERFACE include)

option(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES "Build example programs with graphics that are dependent on axxegro" ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

add_custom_target(graph2x_examples ALL)

add_custom_command(TARGET graph2x_examples POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/examples/resources" "${CMAKE_CURRENT_BINARY_DIR}"
)


function(graph2x_add_example name)
	add_executable("g2x_ex_${name}" "examples/${name}.cpp")
	add_dependencies(graph2x_examples "g2x_ex_${name}")
	target_link_libraries("g2x_ex_${name}" PRIVATE graph2x)
endfunction()

graph2x_add_example("example1")
graph2x_add_example("benchmark")

if(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES)
	add_subdirectory("axxegro")

	graph2x_add_example("graph_editor")
	target_link_libraries(g2x_ex_graph_editor PRIVATE axxegro)
endif ()

#
#add_executable(example1 "examples/example1.cpp")
#target_link_libraries(example1 PRIVATE graph2x)
#
#add_executable(benchmark "examples/benchmark.cpp")
#target_link_libraries(benchmark PRIVATE graph2x)
#
#if(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES)
#	add_subdirectory("axxegro")
#	add_executable(graph_editor "examples/graph_editor.cpp")
#	target_link_libraries(graph_editor PRIVATE graph2x axxegro)
#endif ()



add_executable(mag1_test "tests/mag1.cpp")
target_link_libraries(mag1_test PRIVATE graph2x)

add_executable(hopcroft_karp_vis "tests/hopcroft_karp_vis.cpp")
target_link_libraries(hopcroft_karp_vis PRIVATE graph2x)