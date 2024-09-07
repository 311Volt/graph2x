
#ifndef GRAPH2X_DENSE_GRAPH_HPP
#define GRAPH2X_DENSE_GRAPH_HPP

#include "../core.hpp"
#include "../util.hpp"

namespace g2x {

	enum class dense_graph_kind {
		simple,
		directed,
		bipartite
	};

	template<std::integral VIdxT, dense_graph_kind Kind = dense_graph_kind::simple, bool IsCompact = false>
	class general_dense_graph {
	public:

		static constexpr dense_graph_kind kind = Kind;
		static constexpr bool is_directed = Kind == dense_graph_kind::directed;
		static constexpr bool is_bipartite = Kind == dense_graph_kind::bipartite;
		static constexpr bool natural_vertex_numbering = true;
		static constexpr bool natural_edge_numbering = false;

		using vertex_id_type = VIdxT;

		using edge_value_type = simplified_edge_value<vertex_id_type>;

	private:

		using adj_matrix_element_type = std::conditional_t<IsCompact, bool, boolean>;
		array_2d<adj_matrix_element_type> adj_matrix_;

		[[nodiscard]] bool is_coord_in_unique_region(vertex_id_type u, vertex_id_type v) const {
			if constexpr(kind == dense_graph_kind::simple) {
				return u <= v;
			}
			return true;
		}

		[[nodiscard]] auto&& adj_matrix_ref(vertex_id_type u, vertex_id_type v) const {
			if constexpr(kind == dense_graph_kind::bipartite) {
				return adj_matrix_[u, v-adj_matrix_.width()];
			} else {
				return adj_matrix_[u, v];
			}
		}

	public:

		explicit general_dense_graph(vertex_count num_vertices)
			requires (not is_bipartite)
		: adj_matrix_(num_vertices.value(), num_vertices.value(), false) {

		}

		explicit general_dense_graph(vertex_count num_vertices, std::ranges::forward_range auto&& edges)
			requires (not is_bipartite)
		: general_dense_graph(num_vertices) {

			for(const auto& [u, v]: edges) {
				add_edge(u, v);
			}

		}
		explicit general_dense_graph(vertex_count num_vertices_u, vertex_count num_vertices_v)
					requires is_bipartite
		: adj_matrix_(num_vertices_u.value(), num_vertices_v.value(), false) {

		}

		explicit general_dense_graph(vertex_count num_vertices_u, vertex_count num_vertices_v, std::ranges::forward_range auto&& edges)
			requires is_bipartite
		: general_dense_graph(num_vertices_u, num_vertices_v) {

			for(const auto& [u, v]: edges) {

				add_edge(u, v);
			}

		}




		[[nodiscard]] isize num_vertices_left() const
			requires is_bipartite
		{
			return adj_matrix_.width();
		}
		[[nodiscard]] isize num_vertices_right() const
			requires is_bipartite
		{
			return adj_matrix_.height();
		}


		[[nodiscard]] isize num_vertices() const {
			if constexpr (is_bipartite) {
				return num_vertices_left() + num_vertices_right();
			} else {
				return adj_matrix_.width();
			}
		}

		[[nodiscard]] isize num_edges() const {
			isize result = 0;
			adj_matrix_.for_each_indexed([&](isize i, isize j, boolean v) {
				if(is_coord_in_unique_region(i, j) && v) {
					++result;
				}
			});
			return result;
		}



		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto edge_at(const std::pair<vertex_id_type, vertex_id_type>& eid) const {
			return edge_value_type {eid.first, eid.second};
		}


		[[nodiscard]] auto outgoing_edges(vertex_id_type v, bool unique = false) const {
			if constexpr (kind == dense_graph_kind::bipartite) {
				if(v < num_vertices_left()) {
					return std::views::iota(isize(0), num_vertices_right())
						| std::views::filter([&](isize){return })
				}
			} else {
				return std::views::iota(0, (unique) ? v : num_vertices())
				| std::views::transform([&](int i){
					return edge_value_type {v, i};
				})
				| std::views::filter([&](const edge_value_type& e) {
					return is_adjacent(e.u, e.v);
				});
			}

		}

		[[nodiscard]] bool is_adjacent(int u, int v) const {
			return adj_matrix_ref(u, v);
		}

		[[nodiscard]] auto all_edges() const {
			return std::views::iota(0, num_vertices())
				| std::views::transform([&](int i){
					return outgoing_edges(i, true);
				})
				| std::views::join;
		}

		void add_edge(vertex_id_type u, vertex_id_type v) {
			adj_matrix_ref(u, v) = true;
			if constexpr (kind == dense_graph_kind::simple) {
				adj_matrix_ref(v, u) = true;
			}
		}

		void remove_edge(int u, int v) {
			adj_matrix_ref(u, v) = false;
			if constexpr (kind == dense_graph_kind::simple) {
				adj_matrix_ref(v, u) = false;
			}
		}

		[[nodiscard]] const auto& adjacency_matrix() const {
			return adj_matrix_;
		}

	};
	static_assert(graph<general_dense_graph<int>>);

	using dense_graph = general_dense_graph<int, dense_graph_kind::simple, false>;
	using dense_digraph = general_dense_graph<int, dense_graph_kind::directed, false>;
	using dense_bipartite_graph = general_dense_graph<int, dense_graph_kind::bipartite, false>;

	using compact_dense_graph = general_dense_graph<int, dense_graph_kind::simple, true>;
	using compact_dense_digraph = general_dense_graph<int, dense_graph_kind::directed, true>;
	using compact_dense_bipartite_digraph = general_dense_graph<int, dense_graph_kind::directed, true>;

}

#endif //GRAPH2X_DENSE_GRAPH_HPP
