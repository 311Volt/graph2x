
#ifndef GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA
#define GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA

#include "../core.hpp"

#include <unordered_map>
#include <unordered_set>
#include <ranges>

namespace g2x {

	/*
	 * A fully mutable graph.
	 *
	 * Creation: O(n+m) avg
	 * Adjacency check: O(1) avg
	 * Pass over outgoing_edges: O(N(v)) avg
	 * Pass over adjacent_vertices: O(N(v)) avg
	 * Pass over all_vertices: O(n) avg
	 * Pass over all_edges: O(e) avg
	 * Edge index lookup: O(1) avg
	 */

	template<typename VIdxT, typename EIdxT, bool IsDirected = false>
	class general_dynamic_graph {
	public:

		static constexpr bool is_directed = IsDirected;
		static constexpr bool allows_loops = true;
		static constexpr bool allows_multiple_edges = true;

		static constexpr bool has_natural_vertex_numbering = false;
		static constexpr bool has_natural_edge_numbering = false;
		static constexpr bool outgoing_edges_uv_sorted = false;

		using edge_value_type = edge_value<VIdxT, EIdxT>;
		using vertex_id_type = typename edge_value_type::vertex_id_type;
		using edge_id_type = typename edge_value_type::edge_id_type;


		[[nodiscard]] auto all_vertices() const {
			return std::views::keys(adjacency_);
		}

		[[nodiscard]] auto all_edges() const {
			return std::views::values(edges_);
		}

		[[nodiscard]] auto edge_at(const edge_id_type& eid) const {
			return edges_.at(eid);
		}

		[[nodiscard]] auto is_adjacent(const vertex_id_type& u, const vertex_id_type& v) {
			return adjacency_.at(u).contains(v);
		}

		[[nodiscard]] auto outgoing_edges(const vertex_id_type& vtx) const {
			return adjacency_.at(vtx) | std::views::transform([&](const edge_id_type& eid) {
				return edge_at(eid);
			});
		}

	private:

		std::unordered_map<VIdxT, std::unordered_multiset<EIdxT>> adjacency_;
		std::unordered_map<EIdxT, edge_value_type> edges_;
	};

}

#endif //GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA
