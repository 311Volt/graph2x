
#ifndef GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20
#define GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20

#include <limits>
#include <ostream>
#include <print>
#include <set>

#include "search.hpp"
#include "../core.hpp"

namespace g2x {
	
	namespace algo {
		


		template<typename GraphT>
		auto bipartite_decompose(GraphT&& graph) {
			
			auto labels = create_vertex_label_container<char>(graph, -1);
			
			using result_type = std::optional<decltype(labels)>;
			breadth_first_search bfs(graph);
			
			for(const auto& vtx: all_vertices(graph)) {
				if(bfs.get_vertex_state(vtx) == vertex_search_state::unvisited) {
					labels[vtx] = 0;
					bfs.add_vertex(vtx);
				}
				while(auto opt_vtx = bfs.next_vertex()) {
					const auto& u = *opt_vtx;
					for(const auto& v: adjacent_vertices(graph, u)) {
						if(labels[v] >= 0 && labels[v] == labels[u]) {
							return result_type{std::nullopt}; //odd cycle detected - graph is not bipartite
						}
						labels[v] = !labels[u];
					}
				}
			}
			
			return result_type{labels};
		}

		template<typename GraphT>
		auto find_bipartite_augmenting_path(GraphT&& graph, auto&& partitions, auto&& matching)
			-> std::vector<edge_id_t<GraphT>>
		{
			auto edge_predicate = [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				if(matching[i]) {
					return partitions[u] == 1 && partitions[v] == 0;
				} else {
					return partitions[u] == 0 && partitions[v] == 1;
				}
			};

			breadth_first_search bfs {graph, edge_predicate};

			auto vtx_matched = create_vertex_label_container(graph, char(false));

			for(const auto& [u, v, i]: all_edges(graph)) {
				if(matching[i]) {
					vtx_matched[u] = true;
					vtx_matched[v] = true;
				}
			}

			for(const auto& v: all_vertices(graph)) {
				if(not vtx_matched[v] && partitions[v] == 0) {
					bfs.add_vertex(v);
				}
			}

			while(auto v_opt = bfs.next_vertex()) {
				auto v = *v_opt;
				if(not vtx_matched[v] && partitions[v] == 1) { //augmenting path found - trace to source
					std::vector<edge_id_t<GraphT>> result;
					bfs.trace_path(v, std::back_inserter(result));
					return result;
				}
			}
			return {};
		}


		template<typename GraphT>
		bool is_edge_set_matching(GraphT&& graph, auto&& edge_set) {
			std::set<vertex_id_t<GraphT>> endpoints;
			for(const auto& [u, v, i]: all_edges(graph)) {
				if(not edge_set[i]) {
					continue;
				}
				if(not endpoints.insert(u).second || not endpoints.insert(v).second) {
					return false;
				}
			}
			return true;
		}

		template<typename GraphT>
		bool is_edge_set_maximum_matching(GraphT&& graph, auto&& edge_set) {
			auto partitions = bipartite_decompose(graph).value();
			auto augpath = find_bipartite_augmenting_path(graph, partitions, edge_set);
			// std::println("augpath size {}", augpath.size());
			return augpath.empty();
		}


		
		
		namespace detail {


			
			/* Runs the BFS stage of the Hopcroft-Karp algorithm and returns a vertex-label-container of
			 * BFS distances. In particular, unmatched left vertices have a distance of 0.
			 */
			template<typename GraphT>
			auto hopcroft_karp_bfs_stage(GraphT&& graph, auto&& partitions, auto&& matching, int* out_aug_path_length) {
				auto edge_predicate = [&](auto&& edge) {
					const auto& [u, v, i] = edge;
					if(matching[i]) {
						return partitions[u] == 1 && partitions[v] == 0;
					} else {
						return partitions[u] == 0 && partitions[v] == 1;
					}
				};
				
				breadth_first_search bfs {graph, edge_predicate};
				
				
				auto vtx_matched = create_vertex_label_container(graph, char(false));
				auto is_endpoint_candidate = create_vertex_label_container(graph, char(false));
				
				for(const auto& [u, v, i]: all_edges(graph)) {
					if(matching[i]) {
						vtx_matched[u] = true;
						vtx_matched[v] = true;
					}
				}
				
				for(const auto& vtx: all_vertices(graph)) {
					if(partitions[vtx] == 0 && vtx_matched[vtx] == false) {
						bfs.add_vertex(vtx);
					}
				}
				
				int aug_path_length = std::numeric_limits<int>::max();
				
				auto bfs_layer = create_vertex_label_container(graph, -1);
				while(auto v_opt = bfs.next_vertex()) {
					auto v = *v_opt;
					bfs.update_distances(v, bfs_layer);
					
					if(bfs_layer[v] > aug_path_length) { //search is past all shortest augmenting paths and should terminate
						break;
					}
					
					if(partitions[v] == 1 && vtx_matched[v] == false) { //augmenting path endpoint candidate found
						is_endpoint_candidate[v] = true;
						aug_path_length = std::min(aug_path_length, bfs_layer[v]);
					}
				}
				
				for(const auto& v: all_vertices(graph)) {
					if(bfs_layer[v] == aug_path_length && not is_endpoint_candidate[v]) {
						bfs_layer[v] = -1;
					}
				}
				if(out_aug_path_length) {
					*out_aug_path_length = aug_path_length;
				}
				
				return bfs_layer;
			}
			
			template<typename GraphT>
			bool hopcroft_karp_dfs_step(
				GraphT&& graph,
				const auto& matching,
				const auto& bfs_levels,
				const auto& endpoint_candidates,
				const auto& used_vertices,
				auto&& start_vertex,
				std::optional<edge_id_t<GraphT>> source_edge,
				auto&& output_edges)
			{
				bool source_matched = true;
				if(source_edge) {
					source_matched = matching[*source_edge];
				}

				if(endpoint_candidates[start_vertex]) {
					*output_edges++ = *source_edge;
					return true;
				}

				for(const auto& [u, v, i]: outgoing_edges(graph, start_vertex)) {
					if(not used_vertices[v] && bfs_levels[v] - bfs_levels[u] == 1 && matching[i] != source_matched) {
						const auto& [u1, v1, i1] = edge_at(graph, i);
						if(hopcroft_karp_dfs_step(graph, matching, bfs_levels, endpoint_candidates, used_vertices, v, i, output_edges)) {
							if(source_edge) {
								const auto& [u2, v2, i2] = edge_at(graph, *source_edge);
								*output_edges++ = *source_edge;
							}
							return true;
						}
					}
				}

				return false;

			}
			
			template<typename GraphT>
			void hopcroft_karp_dfs_stage(
				GraphT&& graph,
				const auto& bfs_levels,
				const auto& start_vertices,
				const auto& endpoint_candidates,
				auto&& output_augmenting_set,
				auto&& matching)
			{
				std::vector<edge_id_t<GraphT>> augpath;
				auto used_vertices = create_vertex_label_container<char>(graph, 0);
				
				int dbg_num_aug_paths = 0;
				std::optional<vertex_id_t<GraphT>> dbg_endpoint;
				for(const auto& v: all_vertices(graph)) {
					if(endpoint_candidates[v]) {
						dbg_endpoint = v;
					}
				}
				
				for(const auto& start_vtx: start_vertices) {
					if(used_vertices[start_vtx]) {
						continue;
					}

					augpath.clear();
					if(hopcroft_karp_dfs_step(
							graph, matching, bfs_levels, endpoint_candidates,
							used_vertices, start_vtx, std::nullopt, std::back_inserter(augpath)))
					{
						for(const auto& i: augpath) {
							const auto& [u, v, _] = edge_at(graph, i);
							used_vertices[u] = true;
							used_vertices[v] = true;
							// std::println("consuming edge {}({}), {}({}), #{} (matched={})",
							// 	u, bfs_levels[u], v, bfs_levels[v], i, bool(matching[i]));
							*output_augmenting_set++ = i;
						}
						dbg_num_aug_paths++;
					}

				}
				
				// std::println("debug: {} augmenting paths found (length {})", dbg_num_aug_paths, bfs_levels[*dbg_endpoint]);
			}
			
		}
		
		template<typename GraphT>
		auto find_bipartite_augmenting_set(GraphT&& graph, auto&& partitions, auto&& matching) {
			
			int aug_path_length = -1;
			auto bfs_levels = detail::hopcroft_karp_bfs_stage(graph, partitions, matching, &aug_path_length);
			
			std::vector<vertex_id_t<GraphT>> start_vertices;
			auto endpoint_candidates = create_vertex_label_container(graph, char(false));
			bool endpoint_candidates_exist = false;
			
			for(const auto& vtx: all_vertices(graph)) {
				if(bfs_levels[vtx] == 0) {
					start_vertices.push_back(vtx);
				}
				if(bfs_levels[vtx] == aug_path_length) {
					endpoint_candidates[vtx] = true;
					endpoint_candidates_exist = true;
				}
			}

			if(not endpoint_candidates_exist) {
				return std::vector<edge_id_t<GraphT>>{};
			}
			
			std::vector<edge_id_t<GraphT>> augmenting_set;
			detail::hopcroft_karp_dfs_stage(graph, bfs_levels, start_vertices, endpoint_candidates, std::back_inserter(augmenting_set), matching);
			
			return augmenting_set;
			
		}
		
		template<typename GraphT>
		auto max_bipartite_matching(GraphT&& graph) {
			
			auto partitions = bipartite_decompose(graph).value();
			auto matching = create_edge_label_container(graph, char(false));
			
			while(true) {
				auto aug_set = find_bipartite_augmenting_set(graph, partitions, matching);
				if(aug_set.empty()) {
					break;
				}
				for(const auto& idx: aug_set) {
					matching[idx] = !matching[idx];
				}
				// std::println("matching size: {}, matching valid: {}", std::count(matching.begin(), matching.end(), 1), is_edge_set_matching(graph, matching));
			}
			
			return matching;
		}
		
		template<typename GraphT>
		auto max_weight_bipartite_matching(GraphT&& graph, auto&& weights) {
		
		}
		
		template<typename GraphT>
		auto cycle_cover(GraphT&& graph) {
		
		}



		
	}
	
}

#endif //GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20
