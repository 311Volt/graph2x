
#include <graph2x.hpp>
#include "graph2x_lab/graph2x_lab.hpp"


struct test_red_biconnected_edge_inflation {

	int num_vertices;

	struct x_axis {
		int num_edges;
	};

	struct y_axis {
		int output_num_edges;
	};

	y_axis eval(const x_axis& x, auto&& rng) const {
		auto edges = g2x::graph_gen::edge_cardinality_bipartite_generator(
			num_vertices, num_vertices, x.num_edges, rng);
		auto graph = g2x::create_graph<g2x::dynamic_list_graph>(edges);
		g2x::lab::stopwatch sw;
		g2x::algo::transform_into_biconnected(graph);
		return {
			.output_num_edges = g2x::num_edges(graph)
		};
	}

};


struct test_red_subcubic_vertex_inflation {

	int num_vertices;

	struct x_axis {
		int num_edges;
	};

	struct y_axis {
		int output_num_vertices;
	};

	y_axis eval(const x_axis& x, auto&& rng) const {
		auto edges = g2x::graph_gen::edge_cardinality_bipartite_generator(
			num_vertices, num_vertices, x.num_edges, rng);
		auto graph = g2x::create_graph<g2x::dynamic_list_graph>(edges);
		g2x::lab::stopwatch sw;
		g2x::algo::transform_into_subcubic(graph, [](auto&&){});
		return {
			.output_num_vertices = g2x::num_vertices(graph)
		};
	}

};



int main() {


	g2x::lab::execute_test<test_red_biconnected_edge_inflation>({
		.short_title = "biconnected-edge-inflation_v-400",
		.title = "Przyrost grafu po redukcji usuwającej mosty",
		.samples_per_point = 100,
		.test_instance = {
			.num_vertices = 400
		},
		.x_axis = g2x::lab::logspace(40.0, 25000.0, 100),
		.save_to_csv = true,
		.save_to_pgfplots = true
	});

	g2x::lab::execute_test<test_red_subcubic_vertex_inflation>({
		.short_title = "subcubic-edge-inflation_v-400",
		.title = "Przyrost grafu po redukcji do grafu podkubicznego ($|V| = 400$)",
		.samples_per_point = 100,
		.test_instance = {
			.num_vertices = 400
		},
		.x_axis = g2x::lab::logspace(40.0, 25000.0, 100),
		.save_to_csv = true,
		.save_to_pgfplots = true
	});
}h