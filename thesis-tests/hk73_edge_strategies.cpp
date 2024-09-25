

#include <fstream>

#include "common.hpp"
#include "graph2x_lab/graph2x_lab.hpp"


struct hk73_edge_strategies {

	int num_vertices;

	struct x_axis {
		double avg_deg;
	};

	struct y_axis {
		double phases_default;
		double phases_random;
		double phases_heuristic;
	};

	y_axis eval(const x_axis& x, auto&& random) {

		auto edges = g2x::graph_gen::average_degree_bipartite_generator(num_vertices, num_vertices, x.avg_deg, random);
		auto graph = g2x::create_graph<g2x::basic_graph>(edges);

		g2x::lab::stopwatch sw;
		y_axis result;

		g2x::algo::config::hopcroft_karp.edge_choice_strategy = g2x::algo::config::hk73_edge_choice_strategy_t::unspecified;
		sw = {};
		std::ignore = g2x::algo::max_bipartite_matching(graph);
		result.phases_default = g2x::algo::insights::hopcroft_karp.num_iterations;

		g2x::algo::config::hopcroft_karp.edge_choice_strategy = g2x::algo::config::hk73_edge_choice_strategy_t::random;
		sw = {};
		std::ignore = g2x::algo::max_bipartite_matching(graph);
		result.phases_random = g2x::algo::insights::hopcroft_karp.num_iterations;

		g2x::algo::config::hopcroft_karp.edge_choice_strategy = g2x::algo::config::hk73_edge_choice_strategy_t::lowest_ranked_first;
		sw = {};
		std::ignore = g2x::algo::max_bipartite_matching(graph);
		result.phases_heuristic = g2x::algo::insights::hopcroft_karp.num_iterations;

		return result;
	}

};


int main() {

	g2x::lab::execute_test<hk73_edge_strategies>({
		.short_title = "hk73-edge-strategies",
		.title = "Strategie H-K",
		.samples_per_point = 20000,
		.test_instance = {
			.num_vertices = 500,
		},
		.x_axis = g2x::lab::linspace(2.0, 4.0, 100),
		.save_to_csv = true,
		.save_to_pgfplots = true
	});

}