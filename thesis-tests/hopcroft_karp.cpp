
#include <fstream>

#include "common.hpp"
#include "graph2x_lab/graph2x_lab.hpp"


struct test_hk73_avg_deg_vs_num_phases {

	int num_partition_vertices;

	struct x_axis {
		double average_degree;
	};

	struct y_axis {
		double time_us;
		double num_phases;
	};

	y_axis eval(const x_axis& x, auto&& rng) const {
		auto edges = g2x::graph_gen::average_degree_bipartite_generator(
			num_partition_vertices, num_partition_vertices, x.average_degree, rng);
		auto graph = g2x::create_graph<g2x::basic_graph>(edges);
		g2x::lab::stopwatch sw;
		auto matching = g2x::algo::max_bipartite_matching(graph);
		return {
			.time_us = 1000000.0 * sw.peek(),
			.num_phases = g2x::algo::insights::hopcroft_karp.num_iterations * 1.0,
		};
	}

};

struct test_hk73_num_vtx_vs_num_phases {

	double avg_deg;

	struct x_axis {
		int num_partition_vertices;
	};

	struct y_axis {
		double time_us;
		double num_phases;
	};

	y_axis eval(const x_axis& x, auto&& rng) const {
		auto edges = g2x::graph_gen::average_degree_bipartite_generator(
			x.num_partition_vertices, x.num_partition_vertices, avg_deg, rng);
		auto graph = g2x::create_graph<g2x::basic_graph>(edges);
		g2x::lab::stopwatch sw;
		auto matching = g2x::algo::max_bipartite_matching(graph);
		return {
			.time_us = 1000000.0 * sw.peek(),
			.num_phases = g2x::algo::insights::hopcroft_karp.num_iterations * 1.0,
		};
	}

};

struct test_hk73_graph_type_vs_time {

	double num_partition_vertices;

	struct x_axis {
		double avg_deg;
	};

	struct y_axis {
		double us_time_basic;
		double us_time_basic16;
		double us_time_nestedvec;
		double us_time_dynamic;
	};

	y_axis eval(const x_axis& x, auto&& rng) const {
		auto edges = g2x::graph_gen::average_degree_bipartite_generator(
			num_partition_vertices, num_partition_vertices, x.avg_deg, rng);
		g2x::lab::stopwatch sw;
		y_axis result;
		volatile int sink = 0;

		auto graph1 = g2x::create_graph<g2x::basic_graph>(edges);
		sw = {};
		auto match1 = g2x::algo::max_bipartite_matching(graph1);
		sink += match1.size();
		result.us_time_basic = sw.peek() * 1000.0;

		auto graph2 = g2x::create_graph<g2x::basic_graph_16>(edges);
		sw = {};
		auto match2 = g2x::algo::max_bipartite_matching(graph2);
		sink += match2.size();
		result.us_time_basic16 = sw.peek() * 1000.0;

		auto graph3 = g2x::create_graph<g2x::nested_vec_graph>(edges);
		sw = {};
		auto match3 = g2x::algo::max_bipartite_matching(graph3);
		sink += match3.size();
		result.us_time_nestedvec = sw.peek() * 1000.0;

		auto graph4 = g2x::create_graph<g2x::dynamic_graph>(edges);
		sw = {};
		auto match4 = g2x::algo::max_bipartite_matching(graph4);
		sink += match4.size();
		result.us_time_dynamic = sw.peek() * 1000.0;

		return result;
	}

};


int main() {
goto begin_tests;
	for(int num_vtx: {100, 300, 1000}) {
		for(double focus: {2.0, 0.4}) {
			g2x::lab::execute_test<test_hk73_avg_deg_vs_num_phases>({
				.short_title = std::format("hk73-avg-deg-vs-num-phases_v-{}_f-{:.1f}", num_vtx*2, focus),
				.title = std::format("Liczba faz i czas w zależności od średniego stopnia $G$ ($|V| = {}*2$)", num_vtx),
				.samples_per_point = 160,
				.test_instance = {
					.num_partition_vertices = num_vtx
				},
				.x_axis = g2x::lab::linspace(3.0 - focus, 3.0 + focus, 100),
				.save_to_csv = true,
				.save_to_pgfplots = true
			});
		}
	}

	for(double avg_deg: {1.0, 3.0, 40.0}) {
		g2x::lab::execute_test<test_hk73_num_vtx_vs_num_phases>({
			.short_title = std::format("hk73-num-vtx-vs-num-phases_ad-{:.1f}", avg_deg),
			.title = std::format("Liczba faz i czas w zależności od liczby wierzchołków przy stałym średnim stopniu równym ${:.1f}$", avg_deg),
			.samples_per_point = 500,
			.test_instance = {
				.avg_deg = avg_deg
			},
			.x_axis = g2x::lab::linspace(40.0, 1000.0, 100),
			.save_to_csv = true,
			.save_to_pgfplots = true
		});
	}
begin_tests:
	g2x::lab::execute_test<test_hk73_graph_type_vs_time>({
		.short_title = "hk73-graph-type-vs-runtime",
		.title = "Klasy H-K",
		.samples_per_point = 200,
		.test_instance = {
			.num_partition_vertices = 1000
		},
		.x_axis = g2x::lab::linspace(1.0, 5.0, 100),
		.save_to_csv = true,
		.save_to_pgfplots = true
	});

}