
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

	template<typename VIdxT, std::integral EIdxT = isize, bool IsDirected = false>
	class general_dynamic_graph {
	public:

		static constexpr bool is_directed = IsDirected;
		static constexpr bool allows_loops = true;
		static constexpr bool allows_multiple_edges = true;

		static constexpr bool has_natural_vertex_numbering = false;
		static constexpr bool has_natural_edge_numbering = false;
		static constexpr bool outgoing_edges_uv_sorted = false;

		using edge_value_type = edge_value<VIdxT, EIdxT, IsDirected>;
		using vertex_id_type = typename edge_value_type::vertex_id_type;
		using edge_id_type = typename edge_value_type::edge_id_type;

		explicit general_dynamic_graph(std::ranges::forward_range auto&& edges) {

			for(const auto& [u, v]: edges) {
				add_edge(u, v);
			}

		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::keys(out_adj_index_);
		}

		[[nodiscard]] auto all_edges() const {
			return std::views::values(edges_);
		}

		[[nodiscard]] auto edge_at(const edge_id_type& eid) const {
			return edges_.at(eid);
		}

		[[nodiscard]] auto is_adjacent(const vertex_id_type& u, const vertex_id_type& v) {
			bool result = out_adj_index_.at(u).contains(v);
			if constexpr (not is_directed) {
				result |= in_adj_index_.at(u).contains(v);
			}
			return result;
		}

		[[nodiscard]] auto outgoing_edges(const vertex_id_type& vtx) const {
			return outgoing_edges_source_range(vtx) | std::views::transform([&](auto&& vi) {
				auto [v, i] = vi;
				return edge_value_type{vtx, v, i};
			});
		}

		[[nodiscard]] auto incoming_edges(const vertex_id_type& vtx) const
			requires IsDirected
		{
			return in_adj_index_.at(vtx) | std::views::transform([&](auto&& vi) {
				auto [v, i] = vi;
				return edge_value_type{v, vtx, i};
			});
		}

		void add_vertex(const vertex_id_type& vtx) {
			out_adj_index_[vtx];
			in_adj_index_[vtx];
		}

		bool remove_vertex(const vertex_id_type& vtx) {
			if(not out_adj_index_.contains(vtx)) {
				return false;
			}

			std::vector<EIdxT> eids_to_remove;
			std::ranges::copy(std::views::values(out_adj_index_.at(vtx)), std::back_inserter(eids_to_remove));
			std::ranges::copy(std::views::values(in_adj_index_.at(vtx)), std::back_inserter(eids_to_remove));
			for(auto eid: eids_to_remove) {
				edges_.erase(eid);
			}
			out_adj_index_.erase(vtx);
			in_adj_index_.erase(vtx);

			return true;
		}

		auto add_edge(const vertex_id_type& u, const vertex_id_type& v) {
			auto edge_id = edge_id_counter++;
			add_vertex(u);
			add_vertex(v);
			out_adj_index_[u].insert({v, edge_id});
			if(is_directed || u != v) {
				in_adj_index_[v].insert({u, edge_id});
			}
			edges_[edge_id] = (edge_value_type{u, v, edge_id});
			return edge_id;
		}

		void remove_edge(const edge_id_type& eid) {
			const auto& [u, v, i] = edge_at(eid);
			{
				auto [beg, end] = out_adj_index_[u].equal_range(v);
				auto it = std::ranges::find(beg, end, std::pair{v, eid});
				if(it != end) {
					out_adj_index_[u].erase(it);
				}
			}
			{
				auto [beg, end] = in_adj_index_[v].equal_range(u);
				auto it = std::ranges::find(beg, end, std::pair{u, eid});
				if(it != end) {
					in_adj_index_[v].erase(it);
				}
			}
			edges_.erase(eid);
		}



	private:

		auto outgoing_edges_source_range(const vertex_id_type& vtx) const {
			if constexpr (is_directed) {
				return out_adj_index_.at(vtx);
			} else {
				return
					std::array {
						std::ranges::ref_view(out_adj_index_.at(vtx)),
						std::ranges::ref_view(in_adj_index_.at(vtx))
					}
					| std::views::join;
			}
		}

		edge_id_type edge_id_counter = 0;

		std::unordered_map<VIdxT, std::unordered_multimap<VIdxT, EIdxT>> out_adj_index_;
		std::unordered_map<VIdxT, std::unordered_multimap<VIdxT, EIdxT>> in_adj_index_;
		std::unordered_map<EIdxT, edge_value_type> edges_;
	};

	using dynamic_graph = general_dynamic_graph<int, isize, false>;
	using dynamic_digraph = general_dynamic_graph<int, isize, true>;

}

#endif //GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA
