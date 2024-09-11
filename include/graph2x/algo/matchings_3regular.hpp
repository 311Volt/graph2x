
#ifndef GRAPH2X_MATCHINGS_3REGULAR_HPP
#define GRAPH2X_MATCHINGS_3REGULAR_HPP

#include "search.hpp"
#include "../core.hpp"

namespace g2x {
	namespace algo {

		auto compute_articulation_points_brute_force(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;
			std::vector<std::tuple<vid_t, vid_t, vid_t>> articulation_points;

			vid_t current_vertex{};
			breadth_first_search bfs(graph, [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				return u != current_vertex && v != current_vertex;
			});

			for(const auto& candidate: all_vertices(graph)) {
				current_vertex = candidate;

				std::vector<vid_t> component_roots;
				for(const auto& v: all_vertices(graph)) {
					if(bfs.get_vertex_state(v) == vertex_search_state::unvisited) {
						bfs.add_vertex(v);
						while(bfs.next_vertex()) {}
						component_roots.push_back(v);
					}
				}
				if(component_roots.size() > 1) {
					articulation_points.emplace_back(candidate, component_roots[0], component_roots[1]);
				}
			}

			return articulation_points;
		}

		auto compute_articulation_points(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;

			auto depths = create_vertex_labeling<int>(graph, -1);
			auto lowpoints = create_vertex_labeling<int>(graph, -1);
			auto visited = create_vertex_labeling<boolean>(graph, false);
			std::vector<std::tuple<vid_t, vid_t, vid_t>> articulation_points;

			for(const auto& root: all_vertices(graph)) {
				if(visited[root]) {
					continue;
				}

				[&](this auto&& self, const vid_t* parent, const vid_t& v, int depth) {

					visited[v] = true;
					depths[v] = depth;
					lowpoints[v] = depth;
					int num_children = 0;

					for(const auto& neigh: adjacent_vertices(graph, v)) {
						if(not visited[neigh]) {
							self(&v, neigh, depth+1);
							++num_children;
							lowpoints[v] = std::min(lowpoints[v], lowpoints[neigh]);


						} else if(parent && *parent != neigh) {
							lowpoints[v] = std::min(lowpoints[v], lowpoints[neigh]);
						}
					}

				}(nullptr, root, 0);
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
