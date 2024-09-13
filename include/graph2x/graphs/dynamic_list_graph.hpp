
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
		static constexpr bool allows_loops = true;
		static constexpr bool allows_multiple_edges = true;

		static constexpr bool has_natural_vertex_numbering = false;
		static constexpr bool has_natural_edge_numbering = false;
		static constexpr bool outgoing_edges_uv_sorted = false;
		static constexpr bool outgoing_edges_pre_swapped = true;

		general_dynamic_list_graph(isize num_vertices, std::ranges::forward_range auto&& edges) {
			for(isize i=0; i<num_vertices; i++) {
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
			return std::views::all(active_vertices_);
		}

		[[nodiscard]] auto all_edges() {
			return std::views::all(active_edges_)
				| std::views::transform([&](edge_id_type eid) {
					return edge_at(eid);
				});
		}


		[[nodiscard]] auto&& edge_at(edge_id_type eid) const {
			return *edge_index_[eid]->out_edge_iter;
		}

		[[nodiscard]] auto outgoing_edges(vertex_id_type v) const {
			return outgoing_edges_source_range(v);
		}

		[[nodiscard]] auto incoming_edges(vertex_id_type v) const {
			return std::views::all(in_adj_lists_.at(v));
		}

		edge_id_type create_edge(vertex_id_type v1, vertex_id_type v2) {
			if(not is_vertex_valid(v1) || not is_vertex_valid(v2)) {
				throw std::invalid_argument("inactive vertex id");
			}

			std::list<edge_value_type>& out_list = out_adj_lists_.at(v1);
			std::list<edge_value_type>& in_list = in_adj_lists_.at(v2);

			edge_id_type eid = edge_index_.size();

			auto it1 = out_list.insert(out_list.end(), edge_value_type{v1, v2, eid});
			auto it2 = in_list.end();
			if(is_directed || v1 != v2) {
				it2 = in_list.insert(in_list.end(), edge_value_type{v2, v1, eid});
			}

			auto eit = active_edges_.insert(active_edges_.end(), eid);

			edge_index_.push_back(edge_index_node{it1, it2, eit});
			++num_edges_;
			return eid;
		}

		vertex_id_type create_vertex() {
			vertex_id_type vid = vertex_index_.size();
			auto it = active_vertices_.insert(active_vertices_.end(), vid);
			vertex_index_.push_back(it);
			in_adj_lists_.emplace_back();
			out_adj_lists_.emplace_back();
			++num_vertices_;
			return vid;
		}

		void remove_vertex(vertex_id_type vid) {
			if(not is_vertex_valid(vid)) {
				throw std::invalid_argument("cannot remove invalid vertex");
			}
			in_adj_lists_.at(vid).clear();
			out_adj_lists_.at(vid).clear();
			active_vertices_.erase(vertex_index_.at(vid));
			vertex_index_.at(vid) = std::nullopt;
			--num_vertices_;
		}

		void remove_edge(edge_id_type eid) {
			if(not is_edge_valid(eid)) {
				throw std::invalid_argument("cannot remove invalid edge");
			}
			auto [it1, it2, eit] = edge_index_.at(eid);
			const auto& [v1, v2, i] = *it1;

			std::list<edge_value_type>& out_list = out_adj_lists_.at(v1);
			std::list<edge_value_type>& in_list = in_adj_lists_.at(v2);

			if(it1 != out_list.end()) {
				out_list.erase(it1);
			}
			if(it2 != in_list.end()) {
				in_list.erase(it2);
			}

			active_edges_.erase(eit);
			edge_index_.at(eid) = std::nullopt;
			--num_edges_;
		}


		[[nodiscard]] bool is_vertex_valid(vertex_id_type v) const {
			if(v < 0 || v >= vertex_index_.size()) {
				return false;
			}
			return vertex_index_[v].has_value();
		}

		[[nodiscard]] bool is_edge_valid(edge_id_type eid) const {
			if(eid < 0 || eid >= edge_index_.size()) {
				return false;
			}
			return edge_index_[eid].has_value();
		}

	private:


		auto outgoing_edges_source_range(vertex_id_type vtx) const {
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

		std::vector<std::list<edge_value_type>> in_adj_lists_, out_adj_lists_;
		std::list<vertex_id_type> active_vertices_;
		std::list<edge_id_type> active_edges_;
		using adj_list_iterator = typename std::list<edge_value_type>::iterator;
		using v_list_iterator = typename std::list<vertex_id_type>::iterator;
		using e_list_iterator = typename std::list<edge_id_type>::iterator;

		struct edge_index_node {
			adj_list_iterator out_edge_iter;
			adj_list_iterator in_edge_iter;
			e_list_iterator active_edge_iter;
		};

		std::vector<std::optional<edge_index_node>> edge_index_;
		std::vector<std::optional<v_list_iterator>> vertex_index_;

		isize num_vertices_ = 0;
		isize num_edges_ = 0;

	};

	using dynamic_list_graph = general_dynamic_list_graph<int, int, false>;
	using dynamic_list_digraph = general_dynamic_list_graph<int, int, true>;

}


#endif //GRAPH2X_DYNAMIC_LIST_GRAPH_HPP
