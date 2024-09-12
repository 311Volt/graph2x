
#ifndef GRAPH2X_MATCHINGS_3REGULAR_HPP
#define GRAPH2X_MATCHINGS_3REGULAR_HPP

#include "search.hpp"
#include "../core.hpp"

namespace g2x {
	namespace algo {

		auto compute_articulation_points_brute_force(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;
			std::vector<vid_t> articulation_points;

			std::optional<vid_t> current_candidate;
			breadth_first_search bfs(graph, [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				if(current_candidate) {
					return u != *current_candidate && v != *current_candidate;
				} else {
					return true;
				}
			});

			isize num_connected_components = 0;
			for(const auto& v: all_vertices(graph)) {
				if(bfs.get_vertex_state(v) == vertex_search_state::unvisited) {
					++num_connected_components;
					bfs.add_vertex(v);
					while(bfs.next_vertex());
				}
			}
			bfs.reset();

			for(const auto& candidate: all_vertices(graph)) {
				current_candidate = candidate;
				bfs.reset();

				std::vector<vid_t> component_roots;
				for(const auto& v: all_vertices(graph)) {
					if(v == candidate) {
						continue;
					}
					if(bfs.get_vertex_state(v) == vertex_search_state::unvisited) {
						component_roots.push_back(v);
						bfs.add_vertex(v);
						while(bfs.next_vertex());
					}
				}
				if(component_roots.size() > num_connected_components) {
					articulation_points.emplace_back(candidate);
				}
			}

			return articulation_points;
		}

		auto compute_articulation_points(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;

			auto depths = create_vertex_labeling<int>(graph, -1);
			auto lowpoints = create_vertex_labeling<int>(graph, -1);
			auto visited = create_vertex_labeling<boolean>(graph, false);
			std::vector<vid_t> articulation_points;

			for(const auto& root: all_vertices(graph)) {
				if(visited[root]) {
					continue;
				}

				[&](this auto const& self, const vid_t* parent, const vid_t& v, int depth) -> void {

					visited[v] = true;
					depths[v] = depth;
					lowpoints[v] = depth;
					std::optional<vid_t> v1, v2;

					if(parent) {
						v2 = std::exchange(v1, *parent);
					}

					for(const auto& child: adjacent_vertices(graph, v)) {
						if(not visited[child]) {
							self(&v, child, depth+1);
							if(not parent || lowpoints[child] >= depth) {
								v2 = std::exchange(v1, child);
							}
							lowpoints[v] = std::min(lowpoints[v], lowpoints[child]);
						} else if(parent && *parent != child) {
							lowpoints[v] = std::min(lowpoints[v], depths[child]);
						}
					}
					if(v1 && v2) {
						articulation_points.push_back(v);
					}

				}(nullptr, root, 0);
			}

			return articulation_points;
		}

		/*
		 * Returns a pair (X, Y), where:
		 *
		 * X is a vertex labeling of 'graph' with values of std::optional<int>, where:
		 *    - if v is an articulation point, X[v] is empty
		 *    - otherwise, X[v] is the index of v's biconnected component
		 *
		 * Y is an edge labeling of 'graph' with values of std::optional<int>, where:
		 *    - if e is not adjacent to an articulation point, Y[e] is empty
		 *    - otherwise, Y[e] is the index of e's biconnected component
		 */
		auto biconnected_components_decompose(graph auto&& graph) {
			using vid_t = vertex_id_t<decltype(graph)>;
			auto articulation_points = compute_articulation_points(graph);

			auto is_articulation_point = create_vertex_labeling<boolean>(graph, false);
			auto X = create_vertex_labeling<std::optional<int>>(graph, std::nullopt);
			auto Y = create_edge_labeling<std::optional<int>>(graph, std::nullopt);

			int component_counter = 0;

			depth_first_search dfs(graph, [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				return not is_articulation_point[v]; //can only walk away from articulation points
			});

			for(const auto& ap: articulation_points) {
				is_articulation_point[ap] = true;
				dfs.add_vertex(ap);
			}

			for(const auto& [u, v, i]: dfs.next_edge()) {
				if(not X[u]) {
					int comp = component_counter++;
					X[v] = comp;
					Y[i] = comp;
				} else {
					X[v] = X[u];
					Y[i] = X[u];
				}
			}

			for(const auto& [u, v, i]: all_edges(graph)) {
				if(is_articulation_point[u] && is_articulation_point[v]) {
					Y[i] = component_counter++;
				} else if(is_articulation_point[u] != is_articulation_point[v]) {
					if(X[u]) Y[i] = X[u];
					if(X[v]) Y[i] = X[v];
				}
 			}

			return std::pair{X, Y};

		}

		auto transform_into_biconnected(graph auto&& graph) {

			auto [X, Y] = biconnected_components_decompose(graph);
			int num_biconnected_components = 0;
			for(const auto& e: Y) {
				if(e.has_value()) {
					num_biconnected_components = std::max(num_biconnected_components, 1 + e.value());
				}
			}
			std::vector<boolean> is_reduced(num_biconnected_components, false);





		}

		auto transform_into_subcubic(graph auto&& graph) {

		}

		auto reduce_bipartite_to_biconnected_subcubic(graph auto&& graph) {
			return transform_into_subcubic(transform_into_biconnected(graph));
		}

	}
}

#endif //GRAPH2X_MATCHINGS_3REGULAR_HPP
