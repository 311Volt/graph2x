
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
					skip_distribution = std::geometric_distribution(density)
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

		inline auto iota_random_combination(isize bound, isize samples, auto& generator) {
			auto iota_view = std::views::iota(0, bound);
			std::vector<isize> combination(samples);
			std::ranges::sample(iota_view, combination.begin(), samples, generator);
			return combination;
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




		inline auto edge_probability_generator(isize num_vertices, double density, bool allow_loops, auto&& generator) {
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


		inline auto edge_probability_bipartite_generator(isize v1, isize v2, double density, auto&& generator) {
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

		inline auto edge_cardinality_generator(isize num_vertices, isize num_edges, bool allow_loops, auto&& generator) {
			isize num_edges_complete = detail::complete_graph_num_edges(num_vertices, allow_loops);

			return
				detail::iota_random_combination(num_edges_complete, num_edges, generator)
				| std::views::transform([allow_loops](isize edge_num){
					const auto& [k, t_k] = detail::triangular_floor(edge_num);
					isize v = k + (not allow_loops);
					isize u = edge_num - t_k;
					return std::pair{u, v};
				});
		}


		inline auto edge_cardinality_bipartite_generator(isize v1, isize v2, isize num_edges, auto&& generator) {
			isize num_edges_complete = v1*v2;

			return
				detail::iota_random_combination(num_edges_complete, num_edges, generator)
				| std::views::transform([v1, v2](isize edge_num){
					isize u = edge_num / v2;
					isize v = edge_num % v2 + v1;
					return std::pair{u, v};
				});

		}

		inline auto average_degree_generator(isize num_vertices, double avg_deg, bool allow_loops, auto&& generator) {
			isize num_edges = avg_deg*num_vertices / 2.0;
			return edge_cardinality_generator(num_vertices, num_edges, allow_loops, generator);
		}

		inline auto average_degree_bipartite_generator(isize v1, isize v2, double avg_deg, auto&& generator) {
			isize num_edges = avg_deg*(v1+v2) / 2.0;
			return edge_cardinality_bipartite_generator(v1, v2, num_edges, generator);
		}


	}

}



#endif //GRAPH2X_GRAPH_GEN_BIPARTITE_HPP
