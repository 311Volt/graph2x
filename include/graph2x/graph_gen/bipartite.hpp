
#ifndef GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
#define GRAPH2X_GRAPH_GEN_BIPARTITE_HPP

#include "../graphs/static_simple_graph.hpp"
#include <random>

namespace g2x {
	namespace graph_gen {

		inline g2x::static_simple_graph random_edges_bipartite(int v1, int v2, double density, auto&& generator) {
			std::vector<std::pair<int, int>> edges;
			int num_edges_complete = v1*v2;
			edges.reserve(num_edges_complete*density);

			bool edge = std::uniform_real_distribution(0.0, 1.0)(generator) < density;

			for(int i=0; i<num_edges_complete;) {



				if(edge) {

					int run_length = 1 + std::binomial_distribution(1, density)(generator);
					for(int k=0; k<run_length && i+k < num_edges_complete; k++) {
						int u = (i+k) / v2;
						int v = (i+k) % v2 + v1;
						edges.emplace_back(u, v);
						// std::println("[{}x{} d={}] k={} emplacing {},{}", v1, v2, density, k, u, v);
					}

					i += run_length;

				} else {

					int skip_length = 1 + std::negative_binomial_distribution(1, density)(generator);

					// std::println("[{}x{} d={}] skipping {}", v1, v2, density, skip_length);

					i += skip_length;

				}

				edge = !edge;
			}
			auto graph = g2x::static_simple_graph{v1+v2, edges};
			return graph;
		}

		inline g2x::static_simple_graph random_edges_bipartite_card(int v1, int v2, int num_edges, auto&& generator) {
			double density = num_edges / (1.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

		inline g2x::static_simple_graph random_edges_bipartite_deg(int v1, int v2, double avg_deg, auto&& generator) {
			double density = avg_deg * (v1+v2) / (2.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

	}

}



#endif //GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
