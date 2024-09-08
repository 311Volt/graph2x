
#ifndef GRAPH2X_DENSE_GRAPH_HPP
#define GRAPH2X_DENSE_GRAPH_HPP

#include "../core.hpp"
#include "../util.hpp"

namespace g2x {

	template<std::integral VIdxT, bool IsDirected = false, bool IsCompact = false>
	class general_dense_graph {
	public:

		static constexpr bool is_directed = IsDirected;
		static constexpr bool natural_vertex_numbering = true;
		static constexpr bool natural_edge_numbering = false;

		using vertex_id_type = VIdxT;

		using edge_value_type = simplified_edge_value<vertex_id_type>;

	private:

		using adj_matrix_element_type = std::conditional_t<IsCompact, bool, boolean>;
		array_2d<adj_matrix_element_type> adj_matrix_;

		[[nodiscard]] bool is_coord_in_unique_region(vertex_id_type u, vertex_id_type v) const {
			if constexpr(not IsDirected) {
				return u <= v;
			}
			return true;
		}

		[[nodiscard]] auto&& adj_matrix_ref(this auto&& self, vertex_id_type u, vertex_id_type v) {
			return self.adj_matrix_[u, v];
		}

	public:

		explicit general_dense_graph(vertex_count num_vertices)
		: adj_matrix_(num_vertices.value(), num_vertices.value(), false) {

		}

		explicit general_dense_graph(vertex_count num_vertices, std::ranges::forward_range auto&& edges)
		: general_dense_graph(num_vertices) {
			for(const auto& [u, v]: edges) {
				if(u < 0 || u >= num_vertices.value() || v < 0 || v >= num_vertices.value()) {
					throw std::out_of_range(std::format("invalid edge ({}, {}) in a {}-vertex graph", u, v, num_vertices.value()));
				}
				add_edge(u, v);
			}
		}


		[[nodiscard]] isize num_vertices() const {
			return adj_matrix_.width();
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto outgoing_edges(vertex_id_type v) const {
			//TODO replace with a non-allocating version
			std::vector<edge_value_type> edges;

			for(isize i=0; i<num_vertices(); ++i) {
				if(is_adjacent(v, i)) {
					edges.emplace_back(v, i);
				}
			}

			return edges;
		}

		[[nodiscard]] bool is_adjacent(int u, int v) const {
			return adj_matrix_ref(u, v);
		}

		[[nodiscard]] auto all_edges() const {
			//TODO replace with a non-allocating version
			std::vector<edge_value_type> edges;
			adj_matrix_.for_each_indexed([&](isize x, isize y, bool is_adjacent) {
				if(is_adjacent && is_coord_in_unique_region(x, y)) {
					edges.emplace_back(x, y);
				}
			});
			return edges;
		}

		void add_edge(vertex_id_type u, vertex_id_type v) {
			adj_matrix_ref(u, v) = true;
			if constexpr (not IsDirected) {
				adj_matrix_ref(v, u) = true;
			}
		}

		void remove_edge(int u, int v) {
			adj_matrix_ref(u, v) = false;
			if constexpr (not IsDirected) {
				adj_matrix_ref(v, u) = false;
			}
		}

		[[nodiscard]] const auto& adjacency_matrix() const {
			return adj_matrix_;
		}

		template<typename T>
		[[nodiscard]] auto create_edge_labeling() const {
			return array_2d<T>(adj_matrix_.width(), adj_matrix_.height());
		}

	};
	static_assert(graph<general_dense_graph<int>>);

	using dense_graph = general_dense_graph<int, false, false>;
	using dense_digraph = general_dense_graph<int, true, false>;

	using compact_dense_graph = general_dense_graph<int, false, true>;
	using compact_dense_digraph = general_dense_graph<int, true, true>;

}

#endif //GRAPH2X_DENSE_GRAPH_HPP
