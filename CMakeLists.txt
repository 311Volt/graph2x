cmake_minimum_required(VERSION 3.26)
project(graph2x)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE GRAPH2X_HEADERS "include/*.hpp")

add_library(graph2x INTERFACE ${GRAPH2X_HEADERS})
target_include_directories(graph2x INTERFACE include)

option(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES "Build example programs with graphics that are dependent on axxegro" ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_custom_target(graph2x_examples ALL)

add_custom_command(TARGET graph2x_examples POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/examples/resources" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_library(qlibs_reflect INTERFACE)
target_include_directories(qlibs_reflect INTERFACE "reflect")

function(graph2x_add_example name)
	add_executable("g2x_ex_${name}" "examples/${name}.cpp")
	add_dependencies(graph2x_examples "g2x_ex_${name}")
	target_link_libraries("g2x_ex_${name}" PRIVATE graph2x)
endfunction()


graph2x_add_example("example1")
graph2x_add_example("hopcroft_karp_vis")

if(GRAPH2X_BUILD_GRAPHICAL_EXAMPLES)
	add_subdirectory("axxegro")
	graph2x_add_example("graph_editor")
	target_link_libraries(g2x_ex_graph_editor PRIVATE axxegro)
endif ()


add_executable(g2x_thesis_test_hopcroft_karp "thesis-tests/hopcroft_karp.cpp" "thesis-tests/common.hpp")
target_link_libraries(g2x_thesis_test_hopcroft_karp PRIVATE graph2x qlibs_reflect)

add_executable(g2x_thesis_test_reductions "thesis-tests/reductions.cpp" "thesis-tests/common.hpp")
target_link_libraries(g2x_thesis_test_reductions PRIVATE graph2x qlibs_reflect)



add_subdirectory(googletest)
add_subdirectory(tests)
