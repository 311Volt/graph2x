#include <gtest/gtest.h>
#include <graph2x.hpp>

#include <set>


namespace {

	TEST(basic_graph, empty_graph_should_be_empty) {
		g2x::basic_graph graph{3, std::vector<std::pair<int,int>>{}};
		EXPECT_EQ(g2x::num_edges(graph), 0);
	}

	TEST(basic_graph, adjacency_cardinality_should_match_edge_list) {
		g2x::basic_graph graph{4, std::vector<std::pair<int,int>>{
			{0, 1},
			{1, 2},
			{1, 3},
			{2, 0}
		}};
		EXPECT_EQ(std::ranges::size(g2x::adjacent_vertices(graph, 0)), 2);
		EXPECT_EQ(std::ranges::size(g2x::adjacent_vertices(graph, 1)), 3);
		EXPECT_EQ(std::ranges::size(g2x::adjacent_vertices(graph, 2)), 2);
		EXPECT_EQ(std::ranges::size(g2x::adjacent_vertices(graph, 3)), 1);
	}

	TEST(basic_graph, undirected_all_edges_should_not_yield_duplicates) {
		g2x::basic_graph graph{3, std::vector<std::pair<int,int>>{
			{0, 1},
			{1, 2},
			{2, 0}
		}};
		std::multiset<std::tuple<int,int>> edges;
		for(const auto& [u, v, i]: g2x::all_edges(graph)) {
			edges.emplace(u, v);
		}
		EXPECT_EQ(edges.count({1, 2}), 1);
	}

	TEST(basic_graph, undirected_should_handle_multiple_loops) {
		g2x::basic_graph graph{4, std::vector<std::pair<int,int>>{
			{0, 1},
			{1, 2},
			{2, 0},
			{2, 2},
			{2, 2},
			{3, 3}
		}};
		EXPECT_EQ(std::ranges::size(g2x::outgoing_edges(graph, 2)), 4);
		EXPECT_EQ(std::ranges::size(g2x::outgoing_edges(graph, 3)), 1);
	}

	TEST(basic_graph, undirected_adjacency_relation_should_be_symmetric) {
		g2x::basic_graph graph{3, std::vector<std::pair<int,int>>{
				{0, 1},
				{1, 2},
				{2, 0}
		}};

		EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
		EXPECT_TRUE(g2x::is_adjacent(graph, 2, 1));
	}

	TEST(basic_graph, directed_adjacency_relation_should_be_asymmetric) {
		g2x::basic_digraph graph{3, std::vector<std::pair<int,int>>{
			{0, 1},
			{1, 2},
			{2, 0}
		}};

		EXPECT_TRUE(g2x::is_adjacent(graph, 1, 2));
		EXPECT_FALSE(g2x::is_adjacent(graph, 2, 1));
	}


}