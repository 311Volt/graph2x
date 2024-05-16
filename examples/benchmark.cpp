#include <chrono>
#include <graph2x.hpp>
#include <iostream>
#include <set>
#include <random>


double bfs_benchmark(int numVertices, int numEdges) {
	std::set<std::pair<int, int>> edges;
	std::minstd_rand0 gen{std::random_device{}()};

	for(int i=0; i<numEdges; i++) {
		for(int xd=0; xd<20; xd++) {
			int u = std::uniform_int_distribution(0, numVertices-1)(gen);
			int v = std::uniform_int_distribution(0, numVertices-1)(gen);
			if(u != v && !edges.contains({u, v}) && !edges.contains({u, v})) {
				edges.insert({u, v});
				break;
			}
		}
	}

	g2x::static_simple_graph graph(numVertices, edges);

	auto t0 = std::chrono::high_resolution_clock::now();

	g2x::algo::breadth_first_search bfs(graph);
	int components = 0;
	for(const auto& v: g2x::all_vertices(graph)) {
		if(bfs.get_vertex_state(v) == g2x::vertex_search_state::unvisited) {
			components++;
			bfs.add_vertex(v);
		}
		while(bfs.next_vertex());
	}


	auto t1 = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
}


double match_benchmark(int numPartitionVertices, float avg_neigh) {

	std::set<std::pair<int, int>> edges;
	std::minstd_rand0 gen{std::random_device{}()};

	for(int i=0; i<numPartitionVertices; i++) {
		for(int j=0; j<std::normal_distribution<double>(avg_neigh, 1.0)(gen); j++) {
			for(int xd=0; xd<20; xd++) {
				int u = i;
				int v = numPartitionVertices + std::uniform_int_distribution(0, numPartitionVertices-1)(gen);
				if(u != v && !edges.contains({u, v})) {
					edges.insert({u, v});
					break;
				}
			}
		}

	}

	g2x::static_simple_graph graph(numPartitionVertices*2, edges);

	auto t0 = std::chrono::high_resolution_clock::now();

	auto matching = g2x::algo::max_bipartite_matching(graph);
	auto t1 = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
}


int main() {
	printf("bfs %d,%d: %.6f\n", 50000, 25000, bfs_benchmark(50000, 25000));
	printf("bfs %d,%d: %.6f\n", 50000, 200000, bfs_benchmark(50000, 200000));
	printf("bfs %d,%d: %.6f\n", 2000000, 1000000, bfs_benchmark(2000000, 1000000));
	//printf("bfs %d,%d: %.7f\n", 2000000, 10000000, bfs_benchmark(50000, 10000000));


	printf("match V=%d, E=%d: %.2f ms\n", 1000, 1000*1, 1000.0 * match_benchmark(1000, 1));
	printf("match V=%d, E=%d: %.2f ms\n", 1000, 1000*2, 1000.0 * match_benchmark(1000, 2));
	printf("match V=%d, E=%d: %.2f ms\n", 1000, 1000*5, 1000.0 * match_benchmark(1000, 5));
	printf("match V=%d, E=%d: %.2f ms\n", 1000, 1000*200, 1000.0 * match_benchmark(1000, 200));
	printf("match V=%d, E=%d: %.2f ms\n", 4000, 4000*1, 1000.0 * match_benchmark(4000, 1));
	printf("match V=%d, E=%d: %.2f ms\n", 4000, 4000*2, 1000.0 * match_benchmark(4000, 2));
	printf("match V=%d, E=%d: %.2f ms\n", 4000, 4000*5, 1000.0 * match_benchmark(4000, 5));
	printf("match V=%d, E=%d: %.2f ms\n", 4000, 4000*200, 1000.0 * match_benchmark(4000, 200));


}