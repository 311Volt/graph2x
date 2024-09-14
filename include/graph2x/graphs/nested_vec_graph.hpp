
#ifndef GRAPH2X_NESTED_VEC_GRAPH_HPP
#define GRAPH2X_NESTED_VEC_GRAPH_HPP

#include <list>

#include "../core.hpp"

namespace g2x {

	/*
	 * Like general_basic_graph, but can be constructed in linear time,
	 * at the cost of contiguity and ordering of adjacency information.
	 * Traversal may be slightly slower due to reduced memory locality.
	 *
	 * Additionally, new edges and vertices may be created in amortized constant-time.
	 */

	template<typename VIdxT, std::integral EIdxT = int, bool IsDirected = false>
	class general_nested_vec_graph {
	public:
		using edge_value_type = edge_value<VIdxT, EIdxT, IsDirected>;
		using vertex_id_type = typename edge_value_type::vertex_id_type;
		using edge_id_type = typename edge_value_type::edge_id_type;

		static constexpr bool is_directed = IsDirected;
		static constexpr bool allows_loops = true;
		static constexpr bool allows_multiple_edges = true;

		static constexpr bool has_natural_vertex_numbering = true;
		static constexpr bool has_natural_edge_numbering = true;
		static constexpr bool outgoing_edges_uv_sorted = false;

		general_nested_vec_graph(isize num_vertices, std::ranges::forward_range auto&& edges) {
			adj_storage_.resize(num_vertices);

			if constexpr (std::ranges::sized_range<decltype(edges)>) {
				edge_storage_.reserve(std::ranges::size(edges));
			}

			for(const auto& [u, v]: edges) {
				create_edge(u, v);
			}
		}

		[[nodiscard]] isize num_vertices() const {
			return adj_storage_.size();
		}

		[[nodiscard]] isize num_edges() const {
			return edge_storage_.size();
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto&& edge_at(edge_id_type eid) const {
			return edge_storage_.at(eid);
		}

		[[nodiscard]] auto&& all_edges() const {
			return edge_storage_;
		}

		[[nodiscard]] auto outgoing_edges(vertex_id_type u) const {
			return adj_storage_.at(u) | std::views::transform([&](edge_id_type eid) {return edge_at(eid);});
		}

		vertex_id_type create_vertex() {
			auto result = adj_storage_.size();
			adj_storage_.emplace_back();
			return result;
		}

		edge_id_type create_edge(vertex_id_type u, vertex_id_type v) {
			edge_id_type eid = edge_storage_.size();

			adj_storage_.at(u).push_back(eid);
			if(u != v && !is_directed) {
				adj_storage_.at(v).push_back(eid);
			}
			edge_storage_.push_back(edge_value_type{u, v, eid});
			return eid;
		}

	private:
		std::vector<std::vector<EIdxT>> adj_storage_;
		std::vector<edge_value_type> edge_storage_;
	};

	using nested_vec_graph = general_nested_vec_graph<int, int>;
	using nested_vec_graph_16 = general_nested_vec_graph<uint16_t, int>;

	using nested_vec_digraph = general_nested_vec_graph<int, int, true>;
	using nested_vec_digraph_16 = general_nested_vec_graph<uint16_t, int, true>;

}

#endif //GRAPH2X_NESTED_VEC_GRAPH_HPP
