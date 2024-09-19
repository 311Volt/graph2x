
#ifndef GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20
#define GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20

#include <limits>
#include <ostream>
#include <print>
#include <set>
#include <algorithm>
#include <functional>

#include <math.h>
#include <random>

#include "search.hpp"
#include "../core.hpp"

namespace g2x {
	
	namespace algo {
		


		auto bipartite_decompose(graph auto&& graph) {
			
			auto labels = create_vertex_property<char>(graph, -1);
			
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

		template<typename GraphRefT>
			requires graph<std::remove_cvref_t<GraphRefT>>
		auto find_bipartite_augmenting_path(
			GraphRefT&& graph,
			edge_property_of<GraphRefT, char> auto&& partitions,
			edge_property_of<GraphRefT, bool> auto&& matching
		) -> std::vector<edge_id_t<GraphRefT>>
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

			auto vtx_matched = create_vertex_property(graph, char(false));

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
					std::vector<edge_id_t<GraphRefT>> result;
					bfs.trace_path(v, std::back_inserter(result));
					return result;
				}
			}
			return {};
		}


		bool is_edge_set_matching(graph auto&& graph, edge_property_of<decltype(graph), bool> auto&& edge_set) {
			//TODO O(n) version
			std::set<vertex_id_t<decltype(graph)>> endpoints;
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

		bool is_edge_set_maximum_matching(
			graph auto&& graph,
			edge_property_of<decltype(graph), bool> auto&& edge_set
		) {
			auto partitions = bipartite_decompose(graph).value();
			auto augpath = find_bipartite_augmenting_path(graph, partitions, edge_set);
			return augpath.empty();
		}



		namespace insights {
			inline thread_local struct {
				int longest_augmenting_path = 0;
				int num_iterations = 0;
				std::vector<int> aug_path_lengths;
				std::vector<int> aug_set_sizes;
			} hopcroft_karp;
		}

		namespace config {

			enum class hk73_edge_choice_strategy_t {
				unspecified,
				random,
				lowest_ranked_first
			};

			enum class hk73_vertex_choice_strategy_t {
				unspecified,
				random,
				lowest_ranked_adj_edge_first
			};

			inline thread_local struct {
				hk73_edge_choice_strategy_t edge_choice_strategy = hk73_edge_choice_strategy_t::unspecified;
				hk73_vertex_choice_strategy_t vertex_choice_strategy = hk73_vertex_choice_strategy_t::unspecified;
				struct {
					using result_type = uint64_t;
					[[nodiscard]] static constexpr uint64_t min() {return std::numeric_limits<uint64_t>::min();}
					[[nodiscard]] static constexpr uint64_t max() {return std::numeric_limits<uint64_t>::max();}
					uint64_t operator()() {
						return engine();
					}
					std::function<uint64_t(void)> engine = []() {
						static thread_local std::mt19937_64 gen{std::random_device{}()};
						return gen();
					};
				} random_generator;
			} hopcroft_karp;
		}

		
		namespace detail {




			
			/* Runs the BFS stage of the Hopcroft-Karp algorithm and returns a vertex-label-container of
			 * BFS distances. In particular, unmatched left vertices have a distance of 0.
			 */
			auto hopcroft_karp_bfs_stage(
				graph auto&& graph,
				edge_property_of<decltype(graph), bool> auto&& partitions,
				edge_property_of<decltype(graph), bool> auto&& matching,
				int* out_aug_path_length
			) {
				auto edge_predicate = [&](auto&& edge) {
					const auto& [u, v, i] = edge;
					if(matching[i]) {
						return partitions[u] == 1 && partitions[v] == 0;
					} else {
						return partitions[u] == 0 && partitions[v] == 1;
					}
				};
				
				breadth_first_search bfs {graph, edge_predicate};
				
				
				auto vtx_matched = create_vertex_property<boolean>(graph, false);
				auto is_endpoint_candidate = create_vertex_property<boolean>(graph, false);
				
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
				
				auto bfs_layer = create_vertex_property(graph, -1);
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

			double rate_edge(graph auto&& graph, const edge_t<decltype(graph)>& edge) {
				static constexpr std::array<double, 100> lookup_table = {
					0.000,	0.000,	0.000,	0.000,	0.000,	0.000,	0.000,	0.000,	0.000,	0.000,
					0.000,	0.000,	0.193,	0.308,	0.402,	0.503,	0.586,	0.649,	0.744,	0.784,
					0.000,	0.191,	0.440,	0.497,	0.519,	0.529,	0.501,	0.495,	0.503,	0.483,
					0.000,	0.306,	0.502,	0.502,	0.458,	0.410,	0.370,	0.329,	0.318,	0.280,
					0.000,	0.400,	0.522,	0.458,	0.384,	0.328,	0.273,	0.248,	0.225,	0.194,
					0.000,	0.495,	0.514,	0.416,	0.327,	0.271,	0.219,	0.193,	0.167,	0.148,
					0.000,	0.592,	0.505,	0.364,	0.278,	0.222,	0.180,	0.152,	0.142,	0.112,
					0.000,	0.674,	0.500,	0.357,	0.257,	0.187,	0.159,	0.132,	0.116,	0.096,
					0.000,	0.743,	0.486,	0.312,	0.228,	0.167,	0.144,	0.125,	0.104,	0.085,
					0.000,	0.864,	0.486,	0.302,	0.191,	0.148,	0.112,	0.096,	0.079,	0.061
				};
				const auto& [u, v, _] = edge;


				int degU = std::clamp<int>(std::ranges::distance(outgoing_edges(graph, u)), 0, 9);
				int degV = std::clamp<int>(std::ranges::distance(outgoing_edges(graph, v)), 0, 9);

				int idx = degU*10 + degV;
				return lookup_table[idx];
			}

			bool hopcroft_karp_dfs_step(
				graph auto&& graph,
				edge_property_of<decltype(graph), bool> auto&& matching,
				vertex_property_of<decltype(graph), int> auto&& bfs_levels,
				vertex_property_of<decltype(graph), bool> auto&& endpoint_candidates,
				vertex_property_of<decltype(graph), bool> auto&& used_vertices,
				vertex_id_t<decltype(graph)> start_vertex,
				std::optional<edge_id_t<decltype(graph)>> source_edge,
				std::output_iterator<edge_id_t<decltype(graph)>> auto& output_edges)
			{
				bool source_matched = true;
				if(source_edge) {
					source_matched = matching[*source_edge];
				}

				if(endpoint_candidates[start_vertex]) {
					*output_edges++ = *source_edge;
					return true;
				}

				std::vector<edge_t<decltype(graph)>> out_edges = outgoing_edges(graph, start_vertex) | std::ranges::to<std::vector>();
				using enum config::hk73_edge_choice_strategy_t;
				if(config::hopcroft_karp.edge_choice_strategy == lowest_ranked_first) {
					std::ranges::sort(out_edges, [&](const auto& ea, const auto& eb) {
						return rate_edge(graph, ea) < rate_edge(graph, eb);
					});
				} else if(config::hopcroft_karp.edge_choice_strategy == random) {
					std::ranges::shuffle(out_edges, config::hopcroft_karp.random_generator);
				} else {
					//TODO avoid copy for this case
				}


				for(const auto& [u, v, i]: out_edges/*outgoing_edges(graph, start_vertex)*/) {
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

			void hopcroft_karp_dfs_stage(
				graph auto&& graph,
				vertex_property_of<decltype(graph), int> auto&& bfs_levels,
				g2x::detail::range_of_vertices_for<decltype(graph)> auto&& start_vertices,
				vertex_property_of<decltype(graph), bool> auto&& endpoint_candidates,
				auto&& output_augmenting_set,
				edge_property_of<decltype(graph), bool> auto&& matching)
			{
				std::vector<edge_id_t<decltype(graph)>> augpath;
				auto used_vertices = create_vertex_property<char>(graph, 0);


				
				int dbg_num_aug_paths = 0;
				std::optional<vertex_id_t<decltype(graph)>> dbg_endpoint;
				for(const auto& v: all_vertices(graph)) {
					if(endpoint_candidates[v]) {
						dbg_endpoint = v;
					}
				}

				auto start_vertices_vec = start_vertices | std::ranges::to<std::vector>();
				using enum config::hk73_vertex_choice_strategy_t;
				if(config::hopcroft_karp.vertex_choice_strategy == lowest_ranked_adj_edge_first) {
					auto vertex_ratings = create_vertex_property<double>(graph, 9999.0);
					for(const auto& e: all_edges(graph)) {
						const auto& [u, v, i] = e;
						vertex_ratings[u] = std::min(vertex_ratings[u], rate_edge(graph, e));
						vertex_ratings[v] = std::min(vertex_ratings[v], rate_edge(graph, e));
					}
					std::ranges::sort(start_vertices_vec, [&](const auto& v1, const auto& v2) {
						return vertex_ratings[v1] < vertex_ratings[v2];
					});
				} else if(config::hopcroft_karp.vertex_choice_strategy == random) {
					std::ranges::shuffle(start_vertices_vec, config::hopcroft_karp.random_generator);
				} else {
					//TODO avoid copy for this case
				}

				
				for(const auto& start_vtx: start_vertices_vec /*start_vertices*/) {
					if(used_vertices[start_vtx]) {
						continue;
					}

					augpath.clear();
					auto augpath_back_inserter = std::back_inserter(augpath);
					if(hopcroft_karp_dfs_step(
							graph, matching, bfs_levels, endpoint_candidates,
							used_vertices, start_vtx, std::nullopt, augpath_back_inserter))
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

		auto find_bipartite_augmenting_set(
			graph auto&& graph,
			vertex_property_of<decltype(graph), char> auto&& partitions,
			edge_property_of<decltype(graph), bool> auto&& matching
		) {
			
			int aug_path_length = -1;
			auto bfs_levels = detail::hopcroft_karp_bfs_stage(graph, partitions, matching, &aug_path_length);

			{
				auto& l = insights::hopcroft_karp.longest_augmenting_path;
				l = std::max(aug_path_length, l);
			}
			
			std::vector<vertex_id_t<decltype(graph)>> start_vertices;
			auto endpoint_candidates = create_vertex_property(graph, char(false));
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
				return std::vector<edge_id_t<decltype(graph)>>{};
			}
			
			std::vector<edge_id_t<decltype(graph)>> augmenting_set;
			auto aug_set_output = std::back_inserter(augmenting_set);
			detail::hopcroft_karp_dfs_stage(graph, bfs_levels, start_vertices, endpoint_candidates, aug_set_output, matching);
			
			return augmenting_set;
			
		}

		namespace stats {
			template<typename T>
			struct avg_val {
				T sum{};
				int samples{};

				void add(T val){sum += val; ++samples;}
				[[nodiscard]] T get() const {
					if(samples == 0) {
						return {};
					} else {
						return sum / samples;
					}
				}
			};

			inline thread_local std::array<avg_val<double>, 100> hopcroft_karp_deg_vs_cost;
		}

		auto max_bipartite_matching(graph auto&& graph) {
			
			auto partitions = bipartite_decompose(graph).value();
			auto matching = create_edge_property<boolean>(graph, false);

			insights::hopcroft_karp = {};
			
			while(true) {
				auto aug_set = find_bipartite_augmenting_set(graph, partitions, matching);
				if(aug_set.empty()) {
					break;
				}
				insights::hopcroft_karp.aug_set_sizes.push_back(aug_set.size());
				insights::hopcroft_karp.aug_path_lengths.push_back(insights::hopcroft_karp.longest_augmenting_path);

				for(const auto& idx: aug_set) {
					matching[idx] = !matching[idx];
				}
				++insights::hopcroft_karp.num_iterations;
			}
			
			return matching;
		}

		auto greedy_maximal_matching(graph auto&& graph) {

			auto matching = create_edge_property<boolean>(graph, false);
			auto matched_vertices = create_vertex_property<boolean>(graph, false);

			for(const auto& u: all_vertices(graph)) {
				for(const auto& [_, v, i]: outgoing_edges(graph, u)) {
					if(not matched_vertices[u] && not matched_vertices[v]) {
						matching[i] = true;
						matched_vertices[u] = true;
						matched_vertices[v] = true;
					}
					break;
				}
			}

			return matching;
		}

		auto new_max_bipartite_matching(graph auto&& graph) {

			insights::hopcroft_karp = {};

			auto partitions = bipartite_decompose(graph).value();
			auto matching = create_edge_property<boolean>(graph, false);

			auto bfs_levels = create_vertex_property<int>(graph, -1);
			auto matched_vertices = create_vertex_property<boolean>(graph, false);

			std::vector<edge_id_t<decltype(graph)>> aug_set;
			aug_set.reserve(num_vertices(graph));
			auto aug_set_vtx_map = create_vertex_property<boolean>(graph, false);

			std::vector<vertex_id_t<decltype(graph)>> augpath_begin_candidates;
			augpath_begin_candidates.reserve(num_vertices(graph));

			auto edge_predicate = [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				if(matching[i]) {
					return partitions[u] == 1 && partitions[v] == 0;
				} else {
					return partitions[u] == 0 && partitions[v] == 1;
				}
			};
			auto dfs_stage_edge_predicate = [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				return bfs_levels[v] - bfs_levels[u] == 1 && matching[i] == (bfs_levels[u] % 2);
			};
			auto dfs_stage_vertex_predicate = [&](auto&& vertex) {
				return not aug_set_vtx_map[vertex];
			};
			auto is_vtx_left_unmatched = [&](auto&& v){return partitions[v] == 0 && not matched_vertices[v];};
			auto is_vtx_right_unmatched = [&](auto&& v){return partitions[v] == 1 && not matched_vertices[v];};


			breadth_first_search bfs(graph, edge_predicate);
			depth_first_search dfs(graph, dfs_stage_edge_predicate, dfs_stage_vertex_predicate);

			bfs.expect_up_to(num_vertices(graph));
			dfs.expect_up_to(num_vertices(graph));


			for(int it=0;;it++) {

				// BFS stage

				int phase_aug_path_length = std::numeric_limits<int>::max();

				bfs.reset();
				augpath_begin_candidates.clear();
				for(const auto& v: all_vertices(graph)) {
					bfs_levels[v] = -9999;
					if(is_vtx_left_unmatched(v)) {
						augpath_begin_candidates.push_back(v);
						bfs.add_vertex(v);
					}
				}

				while(auto v_opt = bfs.next_vertex()) {
					auto v = *v_opt;
					bfs.update_distances(v, bfs_levels);
					auto cur_distance = bfs_levels[v];
					if(is_vtx_right_unmatched(v)) {
						if(cur_distance > phase_aug_path_length) {
							break;
						}
						phase_aug_path_length = cur_distance;
					}
				}

				if(phase_aug_path_length == std::numeric_limits<int>::max()) {
					break;
				}

				// DFS stage

				for(const auto& v_begin: augpath_begin_candidates) {

					dfs.reset();
					dfs.add_vertex(v_begin);

					int aug_path_begin = aug_set.size();

					while(auto v_opt = dfs.next_vertex()) {
						if(is_vtx_right_unmatched(*v_opt) && bfs_levels[*v_opt] == phase_aug_path_length) {
							dfs.trace_path(*v_opt, std::back_inserter(aug_set));
							break;
						}
					}
					std::span aug_path {aug_set.begin()+aug_path_begin, aug_set.end()};

					for(const auto& i: aug_path) {
						const auto& [u, v, _] = edge_at(graph, i);
						aug_set_vtx_map[u] = true;
						aug_set_vtx_map[v] = true;
					}

				}


				// flip edges, prepare for next phase

				for(const auto& i: aug_set) {
					matching[i] = !matching[i];

					const auto& [u, v, _] = edge_at(graph, i);
					aug_set_vtx_map[u] = false;
					aug_set_vtx_map[v] = false;

					matched_vertices[u] = true;
					matched_vertices[v] = true;
				}
				aug_set.clear();

				++insights::hopcroft_karp.num_iterations;
			}

			return matching;
		}


		auto max_weight_bipartite_matching(graph auto&& graph, auto&& weights) {
			//TODO
		}

		auto min_bipartite_vertex_cover(graph auto&& graph) {
			//TODO
		}

		auto cycle_cover(graph auto&& graph) {
			//TODO
		}



		
	}
	
}

#endif //GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20
