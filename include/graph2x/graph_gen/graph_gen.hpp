
#ifndef GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
#define GRAPH2X_GRAPH_GEN_BIPARTITE_HPP

#include "../graphs/basic_graph.hpp"
#include <random>

#include <chrono>

namespace g2x {

	namespace detail {
		inline auto iota_random_subset(isize bound, double density, auto&& generator) {
			bool first_present = std::uniform_real_distribution(0.0, 1.0)(generator) < density;

			return rust_like_range(
				[
					&generator,
					bound,
					counter = isize(0),
					run_length_left = isize(first_present),
					skip_length_left = isize(not first_present),
					run_distribution = std::geometric_distribution(1.0 - density),
					skip_distribution = std::geometric_distribution(density),
					density
				]() mutable -> std::optional<isize>
				{
					if(skip_length_left) {
						counter += skip_length_left;
						skip_length_left = 0;
						run_length_left = 1 + run_distribution(generator);
					}
					auto current_element = counter;
					if(run_length_left) {
						if(--run_length_left == 0) {
							skip_length_left = 1 + skip_distribution(generator);
						}
						++counter;
					}
					if(current_element < bound) {
						return current_element;
					} else {
						return std::nullopt;
					}
				}
			);
		}

		/*
		 * Returns the pair (k, T_k), where T_k is the greatest triangular number that is not greater than n.
		 */
		inline std::pair<isize, isize> triangular_floor(isize n) {
			static std::vector<isize> triangular_lookup(1, 0);

			isize k_approx = std::floor(0.5 * (1.0 + std::sqrt(1.0 + 8.0*n))) - 1;

			while(triangular_lookup.size() <= k_approx+2) {
				triangular_lookup.push_back(triangular_lookup.back() + triangular_lookup.size());
			}

			if(triangular_lookup[k_approx] <= n && triangular_lookup[k_approx+1] > n) {
				return {isize(k_approx), triangular_lookup[k_approx]};
			} else {
				isize k = std::distance(triangular_lookup.begin(), std::ranges::upper_bound(triangular_lookup, n));
				k = std::max<isize>(0, k-1);
				return {k, triangular_lookup[k]};
			}
		}

		inline isize complete_graph_num_edges(isize num_vertices, bool allow_loops) {
			if(allow_loops) {
				return (num_vertices * (num_vertices+1)) / 2;
			} else {
				return (num_vertices * (num_vertices-1)) / 2;
			}
		}
	}

	namespace graph_gen {




		inline auto random_edges(isize num_vertices, double density, bool allow_loops, auto&& generator) {
			isize num_edges_complete = detail::complete_graph_num_edges(num_vertices, allow_loops);

			return
				detail::iota_random_subset(num_edges_complete, density, generator)
				| std::views::transform([allow_loops](isize edge_num){
					const auto& [k, t_k] = detail::triangular_floor(edge_num);
					isize v = k + (not allow_loops);
					isize u = edge_num - t_k;
					return std::pair{u, v};
				});
		}

		inline auto random_edges_card(isize num_vertices, isize num_edges, bool allow_loops, auto&& generator) {
			isize num_edges_complete = detail::complete_graph_num_edges(num_vertices, allow_loops);
			double density = num_edges / num_edges_complete;
			return random_edges(num_vertices, density, allow_loops, generator);
		}

		inline auto random_edges_deg(isize num_vertices, double avg_deg, bool allow_loops, auto&& generator) {
			isize num_edges_complete = detail::complete_graph_num_edges(num_vertices, allow_loops);
			double density = avg_deg * num_vertices / (2.0f*num_edges_complete);
			return random_edges(num_vertices, density, allow_loops, generator);
		}


		inline auto random_edges_bipartite(isize v1, isize v2, double density, auto&& generator) {
			std::vector<std::pair<int, int>> edges;
			isize num_edges_complete = v1*v2;
			edges.reserve(num_edges_complete*density);

			return
				detail::iota_random_subset(num_edges_complete, density, generator)
				| std::views::transform([v1, v2](isize edge_num){
					isize u = edge_num / v2;
					isize v = edge_num % v2 + v1;
					return std::pair{u, v};
				});

		}

		inline auto random_edges_bipartite_card(int v1, int v2, int num_edges, auto&& generator) {
			double density = num_edges / (1.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

		inline auto random_edges_bipartite_deg(int v1, int v2, double avg_deg, auto&& generator) {
			double density = avg_deg * (v1+v2) / (2.0f*v1*v2);
			return random_edges_bipartite(v1, v2, density, generator);
		}

	}

}



#endif //GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
