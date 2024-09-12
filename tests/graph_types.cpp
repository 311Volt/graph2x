#include "tests_common.hpp"

namespace {

	using all_graph_type_test_subjects = testing::Types<
		g2x::basic_graph,
		g2x::basic_digraph,
		g2x::dense_graph,
		g2x::dense_digraph,
		g2x::compact_dense_graph,
		g2x::compact_dense_digraph,
		g2x::dynamic_graph,
		g2x::dynamic_digraph,
		g2x::dynamic_list_graph,
		g2x::dynamic_list_digraph,
		g2x::nested_vec_graph,
		g2x::nested_vec_digraph
	>;

	template<typename T>
	using graph_types = testing::Test;

	TYPED_TEST_SUITE(graph_types, all_graph_type_test_subjects);

	TYPED_TEST(graph_types, empty_graph_should_be_empty) {
		using graph_t = TypeParam;
		graph_t graph{3, std::vector<std::pair<int,int>>{}};
		EXPECT_EQ(g2x::num_edges(graph), 0);
	}

	TYPED_TEST(graph_types, undir_adjacency_cardinality_should_match_edge_list) {
		using graph_t = TypeParam;
		if constexpr(g2x::graph_traits::is_directed_v<graph_t> == false) {
			graph_t graph{4, std::vector<std::pair<int,int>>{
					{0, 1},
					{1, 2},
					{1, 3},
					{2, 0}
			}};
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 0)), 2);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 1)), 3);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 2)), 2);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 3)), 1);
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, undirected_all_edges_should_not_yield_duplicates) {
		using graph_t = TypeParam;
		if constexpr(g2x::graph_traits::is_directed_v<graph_t> == false) {
			graph_t graph{3, std::vector<std::pair<int,int>>{
					{0, 1},
					{1, 2},
					{2, 0}
			}};
			std::multiset<std::tuple<int,int>> edges;
			for(const auto& [u, v, i]: g2x::all_edges(graph)) {
				edges.emplace(u, v);
			}
			EXPECT_EQ(edges.count({1, 2}), 1);
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, undirected_should_handle_multiple_loops) {
		//TODO also add tests for loops xor multiple edges
		using graph_t = TypeParam;
		static constexpr bool allows_loops = g2x::graph_traits::allows_loops_v<graph_t>;
		static constexpr bool allows_multiple_edges = g2x::graph_traits::allows_multiple_edges_v<graph_t>;
		if constexpr(g2x::graph_traits::is_directed_v<graph_t> == false && allows_loops && allows_multiple_edges) {
			graph_t graph{4, std::vector<std::pair<int,int>>{
					{0, 1},
					{1, 2},
					{2, 0},
					{2, 2},
					{2, 2},
					{3, 3}
			}};
			EXPECT_EQ(std::ranges::distance(g2x::outgoing_edges(graph, 2)), 4);
			EXPECT_EQ(std::ranges::distance(g2x::outgoing_edges(graph, 3)), 1);
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, undirected_adjacency_relation_should_be_symmetric) {
		using graph_t = TypeParam;
		if constexpr(g2x::graph_traits::is_directed_v<graph_t> == false) {
			graph_t graph{3, std::vector<std::pair<int,int>>{
						{0, 1},
						{1, 2},
						{2, 0}
			}};

			EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
			EXPECT_TRUE(g2x::is_adjacent(graph, 2, 1));
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, directed_adjacency_relation_should_be_asymmetric) {
		using graph_t = TypeParam;
		if constexpr(g2x::graph_traits::is_directed_v<graph_t> == true) {
			graph_t graph{3, std::vector<std::pair<int,int>>{
					{0, 1},
					{1, 2},
					{2, 0}
			}};

			EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
			EXPECT_FALSE(g2x::is_adjacent(graph, 2, 1));
		} else {
			GTEST_SKIP();
		}
	}
}