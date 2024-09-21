
#include <fstream>

#include "common.hpp"


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


auto test_hk73_avg_deg_vs_num_phases(int samples, int part_size, double min, double max, double step) {
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



int main() {
	goto tests_begin;

	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(100, 100, 1.0, 5.0, 0.01),
		"impl1_test_100_coarse.csv"
	);
	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(100, 300, 1.0, 5.0, 0.01),
		"impl1_test_300_coarse.csv"
	);
	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(100, 1000, 1.0, 5.0, 0.01),
		"impl1_test_1000_coarse.csv"
	);


	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(1000, 100, 2.7, 3.3, 0.002),
		"impl1_test_100_fine.csv"
	);
	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(300, 300, 2.7, 3.3, 0.002),
		"impl1_test_300_fine.csv"
	);
	mgr::save_test_results(
		test_hk73_avg_deg_vs_num_phases(100, 1000, 2.7, 3.3, 0.002),
		"impl1_test_1000_fine.csv"
	);
	tests_begin:
	mgr::save_test_results(
		test_hk73_ad3_size_vs_num_phases(33, 10, 3000, 10),
		"impl1_test_size_vs_num_phases.csv"
	);

	mgr::save_test_results(
		test_hk73_ad3_size_vs_max_num_phases(33, 10, 3000, 10),
		"impl1_test_size_vs_max_num_phases.csv"
	);


}