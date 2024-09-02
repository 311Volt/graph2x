
#ifndef GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
#define GRAPH2X_GRAPH_GEN_BIPARTITE_HPP

#include "../graphs/basic_graph.hpp"
#include <random>

#include <chrono>

namespace g2x {
	namespace graph_gen {

		inline g2x::basic_graph random_edges_bipartite(int v1, int v2, double density, auto&& generator) {
			std::vector<std::pair<int, int>> edges;
			int num_edges_complete = v1*v2;
			edges.reserve(num_edges_complete*density);

			auto t0 = std::chrono::high_resolution_clock::now();

			auto run_distribution = std::geometric_distribution(1.0 - density);
			auto skip_distribution = std::geometric_distribution(density);

			bool edge = std::uniform_real_distribution(0.0, 1.0)(generator) < density;

			int xd = 0;

			for(int i=0; i<num_edges_complete;) {
				if(edge) {
					int run_length = 1 + run_distribution(generator);
					for(int k=0; k<run_length && i+k < num_edges_complete; k++) {
						int u = (i+k) / v2;
						int v = (i+k) % v2 + v1;
						edges.emplace_back(u, v);
					}
					xd++;
					i += run_length;
				} else {
					int skip_length = 1 + skip_distribution(generator);
					i += skip_length;
					xd++;
				}

				edge = !edge;
			}
			auto t1 = std::chrono::high_resolution_clock::now();

			auto graph = g2x::basic_graph{v1+v2, edges};

			auto t2 = std::chrono::high_resolution_clock::now();

			double t1_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1-t0).count();
			double t2_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t2-t1).count();
			// std::println("generating {} edges took {} ms ({} gen calls)", edges.size(), t1_ms, xd);
			// std::println("constructing the graph took {} ms", t2_ms);
			return graph;
		}

		inline g2x::basic_graph random_edges_bipartite_card(int v1, int v2, int num_edges, auto&& generator) {
			double density = num_edges / (1.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

		inline g2x::basic_graph random_edges_bipartite_deg(int v1, int v2, double avg_deg, auto&& generator) {
			double density = avg_deg * (v1+v2) / (2.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

	}

}



#endif //GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
