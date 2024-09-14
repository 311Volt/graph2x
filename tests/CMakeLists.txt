
project(graph2x_tests)

add_executable(Google_Tests_run
		graph_types.cpp
		edges.cpp
		graph_generators.cpp
		tests_common.hpp
		matching_reductions.cpp
)

option(GRAPH2X_TESTS_UNITY_BUILD "Enables unity builds for unit tests" ON)

target_link_libraries(Google_Tests_run gtest gtest_main graph2x)
#target_precompile_headers(Google_Tests_run PRIVATE tests_common.hpp)

if(GRAPH2X_TESTS_UNITY_BUILD)
	set_target_properties(Google_Tests_run PROPERTIES UNITY_BUILD ON)
	set_target_properties(Google_Tests_run PROPERTIES UNITY_BUILD_MODE BATCH)
endif ()

target_include_directories(Google_Tests_run PRIVATE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})