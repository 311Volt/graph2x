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

	g2x::static_graph graph(g2x::static_graph::undirected_edgelist_tag_t{}, num_vertices, graph_data);
	std::vector<int> distances(g2x::num_vertices(graph), 0);

	for(const auto& [u, v]: g2x::breadth_first_search(graph)) {
		distances[v] = distances[u]+1;
	}

	for(int i=0; i<distances.size(); i++) {
		printf("v[%d] dist: %d\n", i, distances[i]);
	}
}