#include "tests_common.hpp"

namespace {

	using edge_list = std::vector<std::pair<int, int>>;

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

	TYPED_TEST(graph_types, num_edges_empty) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{});
		EXPECT_EQ(g2x::num_edges(graph), 0);
	}

	TYPED_TEST(graph_types, num_edges) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2},
			{1,2}, {1,3}
		});
		EXPECT_EQ(g2x::num_edges(graph), 4);
	}

	TYPED_TEST(graph_types, num_edges_multiple) {
		if constexpr (g2x::graph_traits::allows_multiple_edges_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {0,2}, {0,2},
				{1,2}, {1,3}
			});
			EXPECT_EQ(g2x::num_edges(graph), 5);
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, num_vertices) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2},
			{1,2}, {1,3}
		});
		EXPECT_GE(g2x::num_vertices(graph), 4);
	}

	TYPED_TEST(graph_types, all_vertices) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2},
			{1,2}, {1,4}
		});
		auto vertices_set = g2x::all_vertices(graph) | std::ranges::to<std::vector>() | std::ranges::to<std::set>();
		for(int v: {0, 1, 2, 4}) {
			EXPECT_TRUE(vertices_set.contains(v));
		}
	}

	TYPED_TEST(graph_types, all_edges) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2},
			{1,2}, {1,3}
		});
		auto edge_set = g2x::simplified(g2x::all_edges(graph)) | std::ranges::to<std::set>();
		EXPECT_TRUE(edge_set.contains({0, 1}));
		EXPECT_TRUE(edge_set.contains({0, 2}));
		EXPECT_TRUE(edge_set.contains({1, 2}));
		EXPECT_TRUE(edge_set.contains({1, 3}));

	}

	TYPED_TEST(graph_types, all_edges_multiple) {
		if constexpr (g2x::graph_traits::allows_multiple_edges_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {0,2}, {0,2},
				{1,2}, {1,3}
			});
			auto edge_multiset = g2x::simplified(g2x::all_edges(graph)) | std::ranges::to<std::multiset>();
			EXPECT_EQ(edge_multiset.count({0, 2}), 2);
		} else {
			GTEST_SKIP();
		}

	}


	TYPED_TEST(graph_types, outgoing_edges) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {2,0},
			{1,2}, {3,1}, {2,4}
		});
		auto edge_multiset = g2x::simplified(g2x::outgoing_edges(graph, 1)) | std::ranges::to<std::multiset>();
		EXPECT_TRUE(edge_multiset.contains({1, 2}));
		if constexpr (not g2x::graph_traits::is_directed_v<TypeParam>) {
			EXPECT_TRUE(edge_multiset.contains({1, 3}));
		}
	}


	TYPED_TEST(graph_types, outgoing_edges_vertex_order) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {2,0},
			{1,2}, {3,1}, {2,4}
		});
		for(const auto& vtx: g2x::all_vertices(graph)) {
			for(const auto& [u, v, i]: g2x::outgoing_edges(graph, vtx)) {
				EXPECT_EQ(u, vtx);
			}
		}
	}


	TYPED_TEST(graph_types, outgoing_edges_multiple) {
		if constexpr (g2x::graph_traits::allows_multiple_edges_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {0,2}, {0,2},
				{1,2}, {3,1}, {2,4}
			});
			auto edge_multiset = g2x::simplified(g2x::outgoing_edges(graph, 0)) | std::ranges::to<std::multiset>();

			EXPECT_EQ(edge_multiset.count({0, 2}), 2);
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, outgoing_edges_loop) {
		if constexpr (g2x::graph_traits::allows_loops_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {2,2},
				{1,2}, {3,1}, {2,4}
			});
			auto edge_multiset = g2x::simplified(g2x::outgoing_edges(graph, 2)) | std::ranges::to<std::multiset>();
			EXPECT_EQ(edge_multiset.count({2, 2}), 1);
		} else {
			GTEST_SKIP();
		}
	}




	TYPED_TEST(graph_types, undir_adjacency_cardinality_should_match_edge_list) {
		if constexpr(g2x::graph_traits::is_directed_v<TypeParam> == false) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{1, 2},
					{1, 3},
					{2, 0}
			});
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 0)), 2);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 1)), 3);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 2)), 2);
			EXPECT_EQ(std::ranges::distance(g2x::adjacent_vertices(graph, 3)), 1);
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, outdegree) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0, 1},
				{1, 3},
				{2, 1}
		});

		if(g2x::graph_traits::is_directed_v<TypeParam>) {
			EXPECT_EQ(g2x::outdegree(graph, 1), 1);
		} else {
			EXPECT_EQ(g2x::degree(graph, 1), 3);
		}
	}

	TYPED_TEST(graph_types, outdegree_multiple) {

		if constexpr (g2x::graph_traits::allows_multiple_edges_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{1, 3},
					{1, 3},
					{2, 1}
			});

			if(g2x::graph_traits::is_directed_v<TypeParam>) {
				EXPECT_EQ(g2x::outdegree(graph, 1), 2);
			} else {
				EXPECT_EQ(g2x::degree(graph, 1), 4);
			}
		} else {
			GTEST_SKIP();
		}


	}

	TYPED_TEST(graph_types, outdegree_loop) {

		if constexpr (g2x::graph_traits::allows_loops_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{2, 2},
					{2, 1},
					{2, 3}
			});

			if(g2x::graph_traits::is_directed_v<TypeParam>) {
				EXPECT_EQ(g2x::outdegree(graph, 2), 3);
			} else {
				EXPECT_EQ(g2x::degree(graph, 2), 4);
			}
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, vertex_labeling) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2}, {0,2},
			{1,2}, {3,1}, {2,4}
		});

		auto l = g2x::create_vertex_labeling<int>(graph, 311);
		for(const auto& v: g2x::all_vertices(graph)) {
			EXPECT_EQ(l[v], 311);
		}
	}


	TYPED_TEST(graph_types, edge_labeling) {
		auto graph = g2x::create_graph<TypeParam>(edge_list{
			{0,1}, {0,2}, {0,2},
			{1,2}, {3,1}, {2,4}
		});

		auto l = g2x::create_edge_labeling<int>(graph, 311);
		for(const auto& [u, v, i]: g2x::all_edges(graph)) {
			EXPECT_EQ(l[i], 311);
		}
	}

	TYPED_TEST(graph_types, create_edge) {
		if constexpr (g2x::graph_traits::supports_edge_creation_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {0,2}, {1,3},
			});
			EXPECT_EQ(g2x::degree(graph, 0), 2);
			g2x::create_edge(graph, 0, 3);
			EXPECT_EQ(g2x::degree(graph, 0), 3);
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, remove_edge) {
		if constexpr (g2x::graph_traits::supports_edge_deletion_v<TypeParam>) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
				{0,1}, {0,2}, {1,3},
			});
			EXPECT_EQ(g2x::degree(graph, 0), 2);
			for(const auto& [u, v, i]: g2x::outgoing_edges(graph, 0)) {
				g2x::remove_edge(graph, i);
				break;
			}
			EXPECT_EQ(g2x::degree(graph, 0), 1);
		} else {
			GTEST_SKIP();
		}

	}

	TYPED_TEST(graph_types, undirected_all_edges_should_not_yield_duplicates) {
		if constexpr(g2x::graph_traits::is_directed_v<TypeParam> == false) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{1, 2},
					{2, 0}
			});
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
		static constexpr bool allows_loops = g2x::graph_traits::allows_loops_v<TypeParam>;
		static constexpr bool allows_multiple_edges = g2x::graph_traits::allows_multiple_edges_v<TypeParam>;
		if constexpr(g2x::graph_traits::is_directed_v<TypeParam> == false && allows_loops && allows_multiple_edges) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{1, 2},
					{2, 0},
					{2, 2},
					{2, 2},
					{3, 3}
			});
			EXPECT_EQ(std::ranges::distance(g2x::outgoing_edges(graph, 2)), 4);
			EXPECT_EQ(std::ranges::distance(g2x::outgoing_edges(graph, 3)), 1);
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, undirected_adjacency_relation_should_be_symmetric) {
		if constexpr(g2x::graph_traits::is_directed_v<TypeParam> == false) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
						{0, 1},
						{1, 2},
						{2, 0}
			});

			EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
			EXPECT_TRUE(g2x::is_adjacent(graph, 2, 1));
		} else {
			GTEST_SKIP();
		}
	}

	TYPED_TEST(graph_types, directed_adjacency_relation_should_be_asymmetric) {
		if constexpr(g2x::graph_traits::is_directed_v<TypeParam> == true) {
			auto graph = g2x::create_graph<TypeParam>(edge_list{
					{0, 1},
					{1, 2},
					{2, 0}
			});

			EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
			EXPECT_FALSE(g2x::is_adjacent(graph, 2, 1));
		} else {
			GTEST_SKIP();
		}
	}
}