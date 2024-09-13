
#ifndef GRAPH2X_DYNAMIC_LIST_GRAPH_HPP
#define GRAPH2X_DYNAMIC_LIST_GRAPH_HPP

#include "../core.hpp"
#include <list>

namespace g2x {

	template<std::integral VIdxT, std::integral EIdxT = int, bool IsDirected = false>
	class general_dynamic_list_graph {
	public:

		using edge_value_type = edge_value<VIdxT, EIdxT, IsDirected>;
		using vertex_id_type = typename edge_value_type::vertex_id_type;
		using edge_id_type = typename edge_value_type::edge_id_type;

		static constexpr bool is_directed = IsDirected;
		static constexpr bool allows_loops = false;
		static constexpr bool allows_multiple_edges = false;

		general_dynamic_list_graph(isize num_vertices, std::ranges::forward_range auto&& edges) {
			for(isize i=0; i<num_vertices_; i++) {
				create_vertex();
			}
			for(const auto& e : edges) {
				const auto& [u, v] = e;
				create_edge(u, v);
			}
		}

		[[nodiscard]] int num_vertices() const {
			return num_vertices_;
		}

		[[nodiscard]] int num_edges() const {
			return num_edges_;
		}

		[[nodiscard]] auto all_vertices() {
			return std::views::iota((isize)0, (isize)vertex_active_.size())
				| std::views::filter([&](const vertex_id_type& v) {
					return is_vertex_valid(v);
				});
		}

		[[nodiscard]] auto all_edges() {
			return std::views::iota((isize)0, (isize)edge_active_.size())
				| std::views::filter([&](const edge_id_type& eid) {
					return is_edge_valid(eid);
				})
				| std::views::transform([&](const edge_id_type& eid) {
					return edge_at(eid);
				});
		}


		[[nodiscard]] auto&& edge_at(const edge_id_type& eid) const {
			return *(edge_index_.at(eid));
		}

		[[nodiscard]] auto outgoing_edges(const vertex_id_type& v) const {
			return outgoing_edges_source_range(v) | std::views::transform([&](const edge_id_type& eid) {
				return edge_at(eid);
			});
		}

		[[nodiscard]] auto incoming_edges(const vertex_id_type& v) const {
			return in_adj_lists_.at(v) | std::views::transform([&](const edge_id_type& eid) {
				return edge_at(eid);
			});
		}

		edge_id_type create_edge(const vertex_id_type& v1, const vertex_id_type& v2) {
			if(not is_vertex_valid(v1) || not is_vertex_valid(v2)) {
				throw std::invalid_argument("inactive vertex id");
			}

			edge_id_type eid = edge_active_.size();
			edge_active_.push_back(eid);
			edge_index_.push_back(edge_list_.insert(edge_list_.end(), edge_value_type{v1, v2, eid}));
			out_adj_lists_.at(v1).push_back(eid);
			in_adj_lists_.at(v2).push_back(eid);
			++num_edges_;
			return eid;
		}

		vertex_id_type create_vertex() {
			vertex_id_type vid = vertex_active_.size();
			vertex_active_.push_back(true);
			in_adj_lists_.emplace_back();
			out_adj_lists_.emplace_back();
			++num_vertices_;
			return vid;
		}

		void remove_vertex(const vertex_id_type& vid) {
			--num_vertices_;
		}

		void remove_edge(const edge_id_type& eid) {
			--num_edges_;
		}

	private:

		[[nodiscard]] bool is_vertex_valid(const vertex_id_type& v) const {
			if(v < 0 || v >= vertex_active_.size()) {
				return false;
			}
			return vertex_active_[v];
		}

		[[nodiscard]] bool is_edge_valid(const edge_id_type& eid) const {
			if(eid < 0 || eid >= edge_active_.size()) {
				return false;
			}
			return edge_active_[eid];
		}

		auto outgoing_edges_source_range(const vertex_id_type& vtx) const {
			if constexpr (is_directed) {
				return out_adj_lists_.at(vtx);
			} else {
				return
					std::array {
						std::ranges::ref_view(out_adj_lists_.at(vtx)),
						std::ranges::ref_view(in_adj_lists_.at(vtx))
					}
				| std::views::join;
			}
		}

		std::list<edge_value_type> edge_list_;
		using list_iterator = typename decltype(edge_list_)::iterator;
		std::vector<list_iterator> edge_index_;
		std::vector<boolean> edge_active_;
		std::vector<boolean> vertex_active_;
		std::vector<std::list<edge_id_type>> in_adj_lists_, out_adj_lists_;

		isize num_vertices_ = 0;
		isize num_edges_ = 0;

	};

	using dynamic_list_graph = general_dynamic_list_graph<int, int, false>;
	using dynamic_list_digraph = general_dynamic_list_graph<int, int, true>;

}


#endif //GRAPH2X_DYNAMIC_LIST_GRAPH_HPP
