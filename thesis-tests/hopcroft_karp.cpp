
#include <fstream>

#include "common.hpp"
#include "graph2x_lab/graph2x_lab.hpp"



struct hk73_result {
	int num_lhs_vertices;
	int num_rhs_vertices;
	int num_edges;
	double avg_deg;
	double avg_time_ms;
	double avg_cardinality;
	double avg_num_phases;
};


auto test_hopcroft_karp(g2x::graph auto&& graph) {
	mgr::stopwatch sw;
	// static volatile unsigned sink = 0;
	auto matching = g2x::algo::max_bipartite_matching(graph);

	double time = sw.peek();
	int cardinality = std::ranges::count(matching, 1);
	int num_phases = g2x::algo::insights::hopcroft_karp.num_iterations;

	return std::tuple{time, cardinality, num_phases};
}


auto test_hk73_avg_deg_vs_num_phases_fn(int samples, int part_size, double min, double max, double step) {
	std::mt19937_64 rng {311};
	std::vector<hk73_result> results;

	for(double avg_deg=min; avg_deg<=max; avg_deg+=step) {
		std::println("ps = {}, ad = {}", part_size, avg_deg);
		mgr::average<double> avg_cardinality, avg_num_phases, avg_time;

		for(int i=0; i<samples; i++) {
			auto edges = g2x::graph_gen::average_degree_bipartite_generator(part_size, part_size, avg_deg, rng);
			auto graph = g2x::create_graph<g2x::basic_graph>(edges);
			const auto [time, cardinality, num_phases] = test_hopcroft_karp(graph);

			avg_time.add(time);
			avg_cardinality.add(cardinality);
			avg_num_phases.add(num_phases);
		}

		results.push_back(hk73_result {
			.num_lhs_vertices = part_size,
			.num_rhs_vertices = part_size,
			.num_edges = int(part_size * avg_deg),
			.avg_deg = avg_deg,
			.avg_time_ms = avg_time.get() * 1000.0,
			.avg_cardinality = avg_cardinality.get(),
			.avg_num_phases = avg_num_phases.get()
		});
	}

	return results;
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

struct hk73_result_2 {
	int num_partition_vertices;
	double avg_deg;
	double avg_num_phases;
};

auto test_hk73_ad3_size_vs_num_phases(int samples, int psize_min, int psize_max, int psize_step) {
	std::mt19937_64 rng {311};
	std::vector<hk73_result_2> results;
	for(int psize=psize_min; psize<=psize_max; psize+=psize_step) {
		std::println("s={}", psize);

		mgr::average<double> avg_num_phases;

		for(int smp=0; smp<samples; smp++) {
			auto edges = g2x::graph_gen::average_degree_bipartite_generator(psize, psize, 3.0, rng);
			auto graph = g2x::create_graph<g2x::basic_graph>(edges);
			const auto [time, cardinality, num_phases] = test_hopcroft_karp(graph);

			avg_num_phases.add(num_phases);
		}

		results.push_back(hk73_result_2 {
			.num_partition_vertices = psize,
			.avg_deg = 3.0,
			.avg_num_phases = avg_num_phases.get()
		});

	}
	return results;
}


struct hk73_result_3 {
	int num_partition_vertices;
	double avg_deg;
	int max_num_phases;
};

auto test_hk73_ad3_size_vs_max_num_phases(int samples, int psize_min, int psize_max, int psize_step) {
	std::mt19937_64 rng {311};
	std::vector<hk73_result_3> results;
	int max_num_phases = 0;
	for(int psize=psize_min; psize<=psize_max; psize+=psize_step) {
		std::println("s={}", psize);

		mgr::average<double> avg_num_phases;

		for(int smp=0; smp<samples; smp++) {
			auto edges = g2x::graph_gen::average_degree_bipartite_generator(psize, psize, 3.0, rng);
			auto graph = g2x::create_graph<g2x::basic_graph>(edges);
			const auto [time, cardinality, num_phases] = test_hopcroft_karp(graph);

			max_num_phases = std::max<int>(max_num_phases, num_phases);
		}

		results.push_back(hk73_result_3 {
			.num_partition_vertices = psize,
			.avg_deg = 3.0,
			.max_num_phases = max_num_phases
		});

	}
	return results;
}



struct hk73_bench_result {
	int num_vertices;
	int num_edges;
	double impl1_avg_time_us;
	double impl2_avg_time_us;
};

auto benchmark_matching_function(int samples, int num_vertices, int num_edges) {
	std::mt19937_64 rng{311};

	static volatile unsigned sink = 0;
	mgr::average<double> avg_time1_s, avg_time2_s;
	for(int i=0; i<samples; i++) {
		auto edges = g2x::graph_gen::edge_cardinality_bipartite_generator(num_vertices/2, num_vertices/2, num_edges, rng);
		auto graph = g2x::create_graph<g2x::basic_graph>(edges);

		mgr::stopwatch sw;
		auto matching1 = g2x::algo::max_bipartite_matching(graph);
		sink += std::ranges::distance(matching1);

		avg_time1_s.add(sw.peek());

		sw = {};
		auto matching2 = g2x::algo::max_bipartite_matching(graph);
		sink += std::ranges::distance(matching2);

		avg_time2_s.add(sw.peek());

	}

	return hk73_bench_result {
		.num_vertices = num_vertices,
		.num_edges = num_edges,
		.impl1_avg_time_us = avg_time1_s.get() * 1000000.0,
		.impl2_avg_time_us = avg_time2_s.get() * 1000000.0
	};
}

auto test_hk73_num_v_vs_time(int samples, double avg_deg, int psize_min, int psize_max, int psize_step) {
	std::vector<hk73_bench_result> results;

	for(int psize=psize_min; psize<psize_max; psize+=psize_step) {
		std::println("s={}", psize);
		results.push_back(benchmark_matching_function(samples, psize*2, psize*avg_deg));
	}

	return results;
}


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



int main() {

	g2x::lab::execute_test<test_hk73_avg_deg_vs_num_phases>({
		.short_title = "hk73-test-test",
		.title = "This is a test",
		.samples_per_point = 50,
		.test_instance = {
			.num_partition_vertices = 100
		},
		.x_axis = [num = 1000]() -> std::generator<test_hk73_avg_deg_vs_num_phases::x_axis> {
			for(int i=0; i<num; i++) {
				co_yield {.average_degree = g2x::lab::linspace_at(1.0, 5.0, num, i)};
			}
		},
		.save_to_csv = true,
		.save_to_pgfplots = true
	});


}