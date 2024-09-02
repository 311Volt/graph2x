
#ifndef GRAPH2X_STATIC_SIMPLE_GRAPH_HPP
#define GRAPH2X_STATIC_SIMPLE_GRAPH_HPP

#include <algorithm>
#include <vector>
#include <span>
#include <set>

#include "../core.hpp"


namespace g2x {
	
	/*
	 * An immutable, undirected, simple graph.
	 *
	 * Creation: O(V + E*log(E))
	 * Adjacency check: O(log(N(v)))
	 * Pass over outgoing_edges: O(N(v))
	 * Pass over adjacent_vertices: O(N(v))
	 * Pass over all_vertices: O(V)
	 * Pass over all_edges: O(E)
	 * Edge index lookup: O(1)
	 */

	class basic_graph {
	public:
		using vertex_id_type = int;
		using edge_id_type = int;

		using edge_value_type = edge_value<vertex_id_type, edge_id_type>;
		using edge_view_type = edge_view<vertex_id_type, edge_id_type>;

		static constexpr bool is_undirected = true;
		static constexpr bool natural_vertex_numbering = true;
		static constexpr bool natural_edge_numbering = true;
	private:
		int num_vertices_;

		std::vector<edge_value_type> edge_storage;

		//the i-th and i+1-th elements denote the index range of adjacency_storage
		std::vector<int> adjacency_regions;

		//i-th element is the index into edge_index_storage to one of the two edges that satisfy i==idx
		std::vector<int> offset_of_edge;

		[[nodiscard]] std::pair<int, int> get_adjacency_range(int v) const {
			return {adjacency_regions.at(v), adjacency_regions.at(v+1)};
		}
	
	public:
		
		basic_graph(int num_vertices, std::ranges::forward_range auto&& edges) {
			this->num_vertices_ = num_vertices;

			int num_edges = 0;

			if constexpr(std::ranges::sized_range<std::remove_cvref_t<decltype(edges)>>) {
				edge_storage.reserve(std::ranges::size(edges));
			}

			for(const auto& [vtx1, vtx2]: edges) {
				if(vtx1 == vtx2) {
					throw std::runtime_error("Loops are not allowed");
				}
				if(vtx1 < 0 || vtx2 < 0 || vtx1 >= num_vertices_ || vtx2 >= num_vertices_) {
					throw std::runtime_error("vertex indices out of range");
				}

				edge_storage.push_back({vtx1, vtx2, num_edges});
				edge_storage.push_back({vtx2, vtx1, num_edges});
				++num_edges;
			}
			std::sort(edge_storage.begin(), edge_storage.end(), [](auto&& e1, auto&& e2) {
				const auto& [u1, v1, i1] = e1;
				const auto& [u2, v2, i2] = e2;
				return std::make_tuple(u1, v1, i1) < std::make_tuple(u2, v2, i2);
			});


			offset_of_edge.resize(num_edges);
			for(int i=edge_storage.size()-1; i>=0; i--) {
				offset_of_edge[edge_storage[i].i] = i;
			}
			

			adjacency_regions.resize(this->num_vertices_ + 1);
			adjacency_regions.front() = 0;
			adjacency_regions.back() = edge_storage.size();
			for(int v=0; v<num_vertices_; v++) {
				if(v > 0) {
					adjacency_regions[v] = adjacency_regions[v-1];
				}
				auto& i = adjacency_regions[v];
				while(i < edge_storage.size() && edge_storage[i].u < v) {
					i++;
				}
			}
			
			
		}
		
		[[nodiscard]] int num_vertices() const {
			return num_vertices_;
		}
		
		[[nodiscard]] int num_edges() const {
			return offset_of_edge.size();
		}

		[[nodiscard]] auto outgoing_edges(int u) const {
			auto [beg, end] = get_adjacency_range(u);

			return std::span{edge_storage.data() + beg, edge_storage.data() + end};
		}
		
		// [[nodiscard]] bool is_adjacent(int u, int v) const {
		// 	static constexpr auto edge_relation = [](const edge_value_type& e1, const edge_value_type& e2) {
		// 		return std::make_tuple(e1.u, e1.v) < std::make_tuple(e2.u, e2.v);
		// 	};
		// 	return std::ranges::binary_search(outgoing_edges(u), edge_value_type{u, v, 0}, edge_relation);
		// }

		// [[nodiscard]] auto adjacent_vertices(int u) const {
		// 	return outgoing_edges(u) | std::views::transform([](const edge_value_type& e){return e.v;});
		// }

		[[nodiscard]] auto edge_at(int index) const {
			return edge_storage[offset_of_edge[index]];
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}
		
		[[nodiscard]] auto all_edges() const {
			return edge_storage | std::views::filter([](const edge_value_type& e){return e.u < e.v;});
		}
		
		// template<typename T>
		// [[nodiscard]] auto create_vertex_labeling(T value = {}) const {
		// 	return std::vector<T>(num_vertices(), value);
		// }
		//
		// template<typename T>
		// [[nodiscard]] auto create_edge_labeling(T value = {}) const {
		// 	return std::vector<T>(num_edges(), value);
		// }
	};
	static_assert(graph<basic_graph>);
	
}

#endif //GRAPH2X_STATIC_SIMPLE_GRAPH_HPP
