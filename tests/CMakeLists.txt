
project(graph2x_tests)

add_executable(Google_Tests_run
		basic_graph.cpp
		dense_graph.cpp
		dynamic_graph.cpp
		edges.cpp
)
target_link_libraries(Google_Tests_run gtest gtest_main graph2x)
target_include_directories(Google_Tests_run PRIVATE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})