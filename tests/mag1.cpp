
#include <graph2x.hpp>

#include <random>
#include <ranges>
#include <algorithm>
#include <chrono>
#include <format>
#include <functional>
#include <print>

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



double benchmark_ms(std::function<int(void)> fn, int num_runs = 100) {
	static volatile int no_opt = 0;
	auto t0 = std::chrono::high_resolution_clock::now();
	for(int i=0; i<num_runs; i++) {
		no_opt = fn();
	}
	auto t1 = std::chrono::high_resolution_clock::now();
	return to_millis(t1 - t0) / num_runs;
}

void basic_hk73_sparse_test(int v1, int v2, float avg_deg) {

	auto graph = graph_gen::random_edges_bipartite_deg(v1, v2, avg_deg);
	auto matching = g2x::create_edge_label_container<char>(graph, false);

	double time_ms = benchmark_ms([&] {
		matching = g2x::algo::max_bipartite_matching(graph);
		return int(matching.size());
	});

	auto match_test = g2x::algo::is_edge_set_matching(graph, matching) ? "OK" : "FAIL";
	auto max_test = g2x::algo::is_edge_set_maximum_matching(graph, matching) ? "OK" : "FAIL";

	auto msize = std::ranges::count(matching, 1);
	std::println("{},{},{},{},{},{:.4f},{},{},{}",
		v1, v2,
		g2x::num_edges(graph),
		g2x::num_edges(graph) / double(v1*v2),
		2*g2x::num_edges(graph) / double(v1+v2),
		time_ms,
		std::ranges::count(matching, 1),
		match_test,
		max_test);
}

void hk73_validity_test(double seconds, int vmin, int vmax, float avg_deg_max) {

	auto t0 = std::chrono::high_resolution_clock::now();



}

int main() {

	std::println("U,V,E,Density,AvgDeg,TimeMs,MatchingSize,MatchTest,MaxTest");

	for(int partition_size = 100; partition_size <= 2000; partition_size += 100) {
		for(float avg_deg = 1.0; avg_deg < 10.0; avg_deg += 0.5) {
			basic_hk73_sparse_test(partition_size, partition_size, avg_deg);
		}
	}
}