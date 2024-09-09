
#ifndef GRAPH2X_MATCHINGS_3REGULAR_HPP
#define GRAPH2X_MATCHINGS_3REGULAR_HPP

#include "search.hpp"
#include "../core.hpp"

namespace g2x {
	namespace algo {

		auto compute_articulation_points(graph auto&& graph) {

			depth_first_search dfs(graph);


			auto depths = create_vertex_labeling<int>(graph, -1);
			auto lowpoints = create_vertex_labeling<int>(graph, -1);
			std::vector<vertex_id_t<decltype(graph)>> dfs_order;

			for(const auto& v: all_vertices(graph)) {
				if(depths[v] != -1) {
					continue;
				}
				dfs.add_vertex(v);
				depths[v] = 0;
				while(auto v_opt = dfs.next_vertex()) {
					if(auto e_opt = dfs.source_edge(*v_opt)) {
						const auto& [u, v, i] = *e_opt;
						depths[v] = depths[u]+1;
					}
					dfs_order.push_back(*v_opt);
				}
			}

			for(const auto& v: std::views::reverse(dfs_order)) {
				if(auto e_opt = dfs.source_edge(v)) {
					const auto& [u, v, i] = *e_opt;
				}
			}


		}

		auto transform_into_biconnected(graph auto&& graph) {

		}

		auto transform_into_subcubic(graph auto&& graph) {

		}

		auto simple_biconnected_3regular_maximum_matching(graph auto&& graph) {
			
		}

		auto fast_biconnected_3regular_maximum_matching(graph auto&& graph) {

		}

		auto reduce_bipartite_to_biconnected_subcubic(graph auto&& graph) {

		}

	}
}

#endif //GRAPH2X_MATCHINGS_3REGULAR_HPP
