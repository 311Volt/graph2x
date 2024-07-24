
#ifndef GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
#define GRAPH2X_GRAPH_GEN_BIPARTITE_HPP

#include "../graphs/static_simple_graph.hpp"
#include <random>

namespace graph_gen {

	inline g2x::static_simple_graph random_edges_bipartite(int v1, int v2, float density, auto&& generator) {
		std::vector<std::pair<int, int>> edges;
		for(int i=0; i<v1; i++) {
			for(int j=0; j<v2; j++) {
				if(std::uniform_real_distribution(0.0f, 1.0f)(generator) < density) {
					edges.emplace_back(i, j+v1);
				}
			}
		}
		auto graph = g2x::static_simple_graph{v1+v2, edges};
		return graph;
	}

	inline g2x::static_simple_graph random_edges_bipartite_card(int v1, int v2, int num_edges, auto&& generator) {
		float density = num_edges / (1.0f*v1*v2);
		return random_edges_bipartite(v1, v2, density, generator);
	}

	inline g2x::static_simple_graph random_edges_bipartite_deg(int v1, int v2, float avg_deg, auto&& generator) {
		float density = avg_deg * (v1+v2) / (2.0f*v1*v2);
		return random_edges_bipartite(v1, v2, density, generator);
	}

}


#endif //GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
