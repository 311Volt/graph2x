
#include "tests_common.hpp"

namespace {

	auto get_random_graph(int v, double d = 3.0) {
		auto random_seed = testing::UnitTest::GetInstance()->random_seed();
		std::println("random_seed = {}", random_seed);
		std::mt19937_64 rng(random_seed);

		auto graph = g2x::create_graph<g2x::basic_graph>(g2x::graph_gen::random_edges_bipartite_deg(v, v, d, rng));
		return graph;
	}


	TEST(matching_reductions, biconnected_reduction) {
		auto graph = get_random_graph(100);

		std::println("graph has {} articulation points", g2x::algo::compute_articulation_points(graph).size());

		auto reduced_graph = g2x::create_graph<g2x::general_dynamic_list_graph<int, int>>(
			g2x::num_vertices(graph),
			g2x::all_edges_unindexed(graph)
		);
		g2x::algo::transform_into_biconnected(reduced_graph);
		auto art_points = g2x::algo::compute_articulation_points(reduced_graph);
		std::println("articulation points: ");
		for(const auto& v: art_points) {
			std::print("{} ", v);
		}
		std::println("");
		EXPECT_EQ(g2x::algo::compute_articulation_points(reduced_graph).size(), 0);
	}

	TEST(matching_reductions, subcubic_reduction) {
		auto graph = get_random_graph(100);

		auto reduced_graph = g2x::create_graph<g2x::general_dynamic_list_graph<int, int>>(graph);
		g2x::algo::transform_into_subcubic(reduced_graph, [](auto&&){});

		for(const auto& v: g2x::all_vertices(reduced_graph)) {
			EXPECT_LE(g2x::degree(reduced_graph, v), 3);
		}
	}


	TEST(matching_reductions, bip_to_biconnected_subcubic_bip_matching) {

		auto graph = get_random_graph(100);

		auto [reduced_graph, reduction_steps] = g2x::algo::reduce_bipartite_to_biconnected_subcubic(graph);

		EXPECT_EQ(g2x::algo::compute_articulation_points(reduced_graph).size(), 0);
		EXPECT_TRUE(g2x::algo::bipartite_decompose(reduced_graph));

		// for(const auto& [u, v, i]: g2x::all_edges(graph)) {
		// 	try {
		// 		const auto& [u1, v1, i1] = g2x::edge_at(reduced_graph, i);
		// 		std::println("edge {}: g {},{},{}, rg {},{},{}", i, u, v, i, u1, v1, i1);
		// 	} catch(const std::exception& e) {
		//
		// 	}
		// }

		EXPECT_TRUE(g2x::algo::is_edge_set_matching(graph, g2x::algo::max_bipartite_matching(graph)));
		EXPECT_TRUE(g2x::algo::is_edge_set_maximum_matching(graph, g2x::algo::max_bipartite_matching(graph)));

		auto matching = g2x::create_edge_labeling<g2x::boolean>(graph, false);
		auto reduced_matching = g2x::algo::max_bipartite_matching(reduced_graph);

		g2x::algo::transfer_matching(graph, matching, reduced_matching, reduction_steps);

		EXPECT_TRUE(g2x::algo::is_edge_set_matching(graph, matching));
		EXPECT_TRUE(g2x::algo::is_edge_set_maximum_matching(graph, matching));


	}




}