
#ifndef GRAPH2X_STATIC_SIMPLE_DIGRAPH_HPP
#define GRAPH2X_STATIC_SIMPLE_DIGRAPH_HPP

#include "../core.hpp"

namespace g2x {

	/*
	 * An immutable, directed, simple graph.
	 *
	 * Creation: O(V + E*log(E))
	 * Adjacency check: O(log(N(v)))
	 * Pass over outgoing_edges: O(N(v))
	 * Pass over adjacent_vertices: O(N(v))
	 * Pass over all_vertices: O(V)
	 * Pass over all_edges: O(E)
	 * Edge index lookup: O(1)
	 */

	class static_simple_digraph {
	private:
		using vertex_id_type = int;
		using edge_id_type = int;

		using edge_view_type = edge_view<vertex_id_type, edge_id_type>;
		using edge_value_type = edge_value<vertex_id_type, edge_id_type>;

		int num_vertices_;

		std::vector<vertex_id_type> source_vtx_storage;
		std::vector<vertex_id_type> dest_vtx_storage;

		std::vector<int> adjacency_regions;

		[[nodiscard]] std::pair<int, int> get_adjacency_range(int v) const {
			return {adjacency_regions.at(v), adjacency_regions.at(v+1)};
		}

	public:
		static_simple_digraph(int num_vertices, std::ranges::range auto&& edges) {
			this->num_vertices_ = num_vertices;

			std::vector<std::pair<vertex_id_type, vertex_id_type>> edge_set;
			if constexpr(std::ranges::sized_range<std::remove_cvref_t<decltype(edges)>>) {
				edge_set.reserve(std::ranges::size(edges));
			}

			for(const auto& [u, v]: edges) {
				edge_set.push_back({u, v});
			}
			std::sort(edge_set.begin(), edge_set.end());

			source_vtx_storage.resize(edge_set.size());
			dest_vtx_storage.resize(edge_set.size());
			for(int i=0; i<edge_set.size(); i++) {
				source_vtx_storage[i] = edge_set[i].first;
				dest_vtx_storage[i] = edge_set[i].second;
			}

			adjacency_regions.resize(this->num_vertices_ + 1);
			adjacency_regions.front() = 0;
			adjacency_regions.back() = source_vtx_storage.size();
			for(int v=0; v<num_vertices_; v++) {
				if(v > 0) {
					adjacency_regions[v] = adjacency_regions[v-1];
				}
				auto& i = adjacency_regions[v];
				while(source_vtx_storage[i] < v) {
					i++;
				}
			}
		}


		[[nodiscard]] int num_vertices() const {
			return num_vertices_;
		}

		[[nodiscard]] int num_edges() const {
			return source_vtx_storage.size();
		}

		[[nodiscard]] bool is_adjacent(int u, int v) const {
			auto [beg, end] = get_adjacency_range(u);
			return std::binary_search(&dest_vtx_storage[beg], &dest_vtx_storage[end], v);
		}

		[[nodiscard]] auto adjacent_vertices(int u) const {
			auto [beg, end] = get_adjacency_range(u);
			return std::span {dest_vtx_storage.data()+beg, dest_vtx_storage.data()+end};
		}

		[[nodiscard]] auto edge_at(int index) const {
			return edge_view_type {source_vtx_storage[index], dest_vtx_storage[index], index};
		}

		[[nodiscard]] auto outgoing_edges(int u) const {
			auto [beg, end] = get_adjacency_range(u);

			return std::views::iota(beg, end) | std::views::transform([&](auto&& idx) {
				return edge_view_type {source_vtx_storage[idx], dest_vtx_storage[idx], idx};
			});
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto all_edges() const {
			return std::views::iota(0, num_edges()) | std::views::transform([this](auto&& idx) {
				return edge_view_type {source_vtx_storage[idx], dest_vtx_storage[idx], idx};
			});
		}

		template<typename T>
		[[nodiscard]] auto create_vertex_labeling(T value = {}) const {
			return std::vector<T>(num_vertices(), value);
		}

		template<typename T>
		[[nodiscard]] auto create_edge_labeling(T value = {}) const {
			return std::vector<T>(num_edges(), value);
		}
	};

}

#endif //GRAPH2X_STATIC_SIMPLE_DIGRAPH_HPP
