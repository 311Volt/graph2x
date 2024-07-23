
#include <graph2x.hpp>

#include <random>
#include <ranges>
#include <algorithm>
#include <chrono>
#include <format>
#include <functional>
#include <print>


void print_bfs_net_to_tikz(std::ostream& output, auto&& graph, auto&& matching, auto&& bfs_levels) {




}


double to_millis(auto&& duration) {
	return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(duration).count();
}

namespace graph_gen {

	g2x::static_simple_graph random_edges_bipartite(int v1, int v2, float density) {
		auto t0 = std::chrono::high_resolution_clock::now();
		std::mt19937_64 gen{std::random_device{}()};
		std::vector<std::pair<int, int>> edges;
		for(int i=0; i<v1; i++) {
			for(int j=0; j<v2; j++) {
				if(std::uniform_real_distribution(0.0f, 1.0f)(gen) < density) {
					edges.emplace_back(i, j+v1);
				}
			}
		}
		auto t1 = std::chrono::high_resolution_clock::now();
		//std::println("generated {} edges", edges.size());
		auto graph = g2x::static_simple_graph{v1+v2, edges};
		auto t2 = std::chrono::high_resolution_clock::now();
		//std::println("edge gen: {:.4f} ms, graph gen: {:.4f} ms", to_millis(t1 - t0), to_millis(t2 - t1));
		return graph;
	}

	g2x::static_simple_graph random_edges_bipartite_card(int v1, int v2, int num_edges) {
		float density = num_edges / (1.0f*v1*v2);
		return random_edges_bipartite(v1, v2, density);
	}

	g2x::static_simple_graph random_edges_bipartite_deg(int v1, int v2, float avg_deg) {
		float density = avg_deg * (v1+v2) / (2.0f*v1*v2);
		return random_edges_bipartite(v1, v2, density);
	}
	
}



double benchmark_ms(const std::function<void(void)>& setup, const std::function<int(void)>& fn, int num_runs = 5) {
	static volatile int no_opt = 0;
	setup();
	auto t0 = std::chrono::high_resolution_clock::now();
	for(int i=0; i<num_runs; i++) {
		no_opt = fn();
	}
	auto t1 = std::chrono::high_resolution_clock::now();
	return to_millis(t1 - t0) / num_runs;
}

void print_header(const std::string& text) {
	std::println("------------------{}------------------", text);
}

void hk73_test_avg_deg_vs_num_iters(int v, float avg_deg_min, float avg_deg_max, float avg_deg_step) {
	int samples_per_point = 30;

	g2x::static_simple_graph graph(0, std::vector<std::pair<int,int>>{});
	print_header("TEST #1");
	std::println("V,E,AvgDeg,NumIters,TimeMs");

	for(float avg_deg=avg_deg_min; avg_deg<=avg_deg_max; avg_deg += avg_deg_step) {
		double time_ms = 0.0;
		double num_iters = 0.0;
		for(int smp=0; smp<samples_per_point; smp++) {
			time_ms += benchmark_ms(
				[&]() {
					graph = graph_gen::random_edges_bipartite_deg(v, v, avg_deg);
				},
				[&]() {
					auto matching = g2x::algo::max_bipartite_matching(graph);
					return std::size(matching);
				},
				1
			);
			num_iters += g2x::algo::insights::hopcroft_karp.num_iterations;
		}
		time_ms /= samples_per_point;
		num_iters /= samples_per_point;
		std::println("{},{},{:.3f},{:.3f},{:.3f}", v, int(double(v)*avg_deg), avg_deg, num_iters, time_ms);
	}

}

void hk73_test_slowest_avg_deg(int v_min, int v_max, int v_step) {

	auto di2avgdeg = [](int di)->float{return 2.5 + 0.01*di;};
	int samples_per_point = 10;
	g2x::static_simple_graph graph(0, std::vector<std::pair<int,int>>{});
	for(int v=v_min; v<=v_max; v+=v_step) {
		std::array<double, 100> num_iters{};


		for(int di=0; di<=100; di++) {
			float avg_deg = di2avgdeg(di);
			for(int smp=0; smp<samples_per_point; smp++) {
				benchmark_ms(
					[&]() {
						graph = graph_gen::random_edges_bipartite_deg(v, v, avg_deg);
					},
					[&]() {
						auto matching = g2x::algo::max_bipartite_matching(graph);
						return std::size(matching);
					},
					1
				);
				num_iters[di] += g2x::algo::insights::hopcroft_karp.num_iterations;
			}
			num_iters[di] /= samples_per_point;
		}


	}

}

void hk73_test_randomized_dfs() {

	auto graph = graph_gen::random_edges_bipartite_deg(2000, 2000, 3.0);
	auto data = g2x::algo::max_bipartite_matching_RANDOMIZED_TEMP(graph, 1000);
	std::println("NumStages,LongestAugPath,AvgAugpathLength,FirstMatchSize");



	for(const auto& run_data: data) {

		auto& al = run_data.augpath_lengths;
		int num_stages = al.size();
		int longest_aug_path = al.back();
		double avg_augpath_length = std::accumulate(al.begin(), al.end(), 0.0) / double(num_stages);


		std::println("{},{},{:.2f},{}", num_stages, longest_aug_path, avg_augpath_length, run_data.first_stage_match_size);

	}

	/*
	 * TODO trzeba przetwarzac caly graf w pozniejszych fazach?
	 * TODO losujemy wiecej na poczatku - czy to sie przeklada na wieksza liczbe faz?
	 * jakis parametr ktoregos ciagu w insights ktory dobrze koreluje sie z iloscia faz? */


	auto match = g2x::algo::max_bipartite_matching(graph);

	std::println("best matching cardinality: {}", std::ranges::count(match, 1));

}



int main() {

	hk73_test_randomized_dfs();

	// hk73_test_avg_deg_vs_num_iters(500, 1.0, 5.0, 0.02);
	//
	// std::println("cost stats: ");
	// for(int u=0; u<10; u++) {
	// 	for(int v=0; v<10; v++) {
	// 		std::print("{:.3f},\t", g2x::algo::stats::hopcroft_karp_deg_vs_cost[u*10+v].get());
	// 	}
	// 	std::println("");
	// }
	//
	// std::println("samples: ");
	// for(int u=0; u<10; u++) {
	// 	for(int v=0; v<10; v++) {
	// 		std::print("{}\t", g2x::algo::stats::hopcroft_karp_deg_vs_cost[u*10+v].samples);
	// 	}
	// 	std::println("");
	// }

	//hk73_test_avg_deg_vs_num_iters(2000, 2.5, 3.5, 0.001);

}