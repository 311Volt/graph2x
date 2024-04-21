#include <iostream>
#include <optional>
#include <vector>
#include <graph2x.hpp>

int main() {
	int num_vertices = 6;
	std::vector<std::pair<int, int>> graph_data = {
		{0, 2},
		{0, 4},
		{0, 5},
		{1, 4},
		{1, 5},
		{2, 3},
		{2, 4},
		{4, 5}
	};

	g2x::static_simple_graph graph(num_vertices, graph_data);
	std::vector<int> distances(graph.num_vertices(), 0);

	for(const auto& [u, v, i]: g2x::algo::breadth_first_search_edges(graph, 0)) {
		distances[v] = distances[u]+1;
	}

	for(int i=0; i<distances.size(); i++) {
		printf("v[%d] dist: %d\n", i, distances[i]);
	}
}