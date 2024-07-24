
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
			double rate_edge(GraphT&& graph, const edge_t<GraphT>& edge) {
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


				int degU = std::clamp<int>(std::ranges::size(outgoing_edges(graph, u)), 0, 9);
				int degV = std::clamp<int>(std::ranges::size(outgoing_edges(graph, v)), 0, 9);

				int idx = degU*10 + degV;
				return lookup_table[idx];
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

				std::vector<edge_t<GraphT>> out_edges = outgoing_edges(graph, start_vertex) | std::ranges::to<std::vector>();
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

				auto start_vertices_vec = start_vertices | std::ranges::to<std::vector>();
				using enum config::hk73_vertex_choice_strategy_t;
				if(config::hopcroft_karp.vertex_choice_strategy == lowest_ranked_adj_edge_first) {
					auto vertex_ratings = create_vertex_label_container<double>(graph, 9999.0);
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

			{
				auto& l = insights::hopcroft_karp.longest_augmenting_path;
				l = std::max(aug_path_length, l);
			}
			
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
		
		template<typename GraphT>
		auto max_bipartite_matching(GraphT&& graph) {
			
			auto partitions = bipartite_decompose(graph).value();
			auto matching = create_edge_label_container(graph, char(false));

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

		template<typename GraphT>
		auto max_bipartite_matching_RANDOMIZED_TEMP(GraphT&& graph, int num_attempts) {

			std::vector<decltype(insights::hopcroft_karp)> results;
			for(int i=0; i<num_attempts; i++) {
				max_bipartite_matching(graph);
				results.push_back(insights::hopcroft_karp);
			}

			return results;
		}


		
		template<typename GraphT>
		auto max_weight_bipartite_matching(GraphT&& graph, auto&& weights) {
			//TODO
		}

		template<typename GraphT>
		auto min_bipartite_vertex_cover(GraphT&& graph) {
			//TODO
		}

		template<typename GraphT>
		auto cycle_cover(GraphT&& graph) {
		
		}



		
	}
	
}

#endif //GRAPH2X_BIP_MATCHINGS_HPP_F55F4E4CE9B4444E87B1AB204C603E20
