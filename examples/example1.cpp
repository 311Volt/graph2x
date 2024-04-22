#include <iostream>
#include <graph2x.hpp>

int main() {

	g2x::static_simple_graph graph(6, std::vector<std::pair<int, int>>{
		{0, 2},
		{0, 4},
		{0, 5},
		{1, 4},
		{1, 5},
		{2, 3},
		{2, 4},
		{4, 5}
	});

	auto distances = g2x::create_vertex_label_container(graph, -1);
	distances[0] = 0;

	for(const auto& [u, v, i]: g2x::algo::simple_edges_bfs(graph, 0)) {
		printf("visiting edge [%d, %d]\n", u, v);
		distances[v] = distances[u]+1;
	}

	for(int i=0; i<distances.size(); i++) {
		printf("v[%d] dist: %d\n", i, distances[i]);
	}


	// https://d3i71xaburhd42.cloudfront.net/8e1a91c72f61515a28b77e0bc443350bbbe00cb1/3-Figure3-1.png
	g2x::static_simple_graph ex_bip_graph(11, std::vector<std::pair<int,int>>{
		{0, 5},
		{0, 6},
		{1, 5},
		{2, 6},
		{2, 7},
		{2, 8},
		{2, 9},
		{3, 7},
		{3, 10},
		{4, 8}
	});

	auto matching = g2x::algo::max_bipartite_matching(ex_bip_graph);

	for(const auto& [u, v, i]: g2x::all_edges(ex_bip_graph)) {
		if(matching[i]) {
			printf("matching edge: [%d, %d (%d)]\n", u, v, i);
		}
	}

}