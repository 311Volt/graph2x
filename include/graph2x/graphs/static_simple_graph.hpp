
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

	class static_simple_graph {
	public:
		using vertex_id_type = int;
		using edge_id_type = int;

		using edge_value_type = edge_value<vertex_id_type, edge_id_type>;
		using edge_view_type = edge_view<vertex_id_type, edge_id_type>;
	private:
		int num_vertices_;

		std::vector<vertex_id_type> source_vtx_storage;
		std::vector<vertex_id_type> dest_vtx_storage;
		std::vector<edge_id_type> edge_idx_storage;

		//the i-th and i+1-th elements denote the index range of adjacency_storage
		std::vector<int> adjacency_regions;

		//i-th element is the index into edge_index_storage to one of the two edges that satisfy i==idx
		std::vector<int> offset_of_edge;

		[[nodiscard]] std::pair<int, int> get_adjacency_range(int v) const {
			return {adjacency_regions.at(v), adjacency_regions.at(v+1)};
		}
	
	public:
		
		static_simple_graph(int num_vertices, std::ranges::forward_range auto&& edges) {
			this->num_vertices_ = num_vertices;

			int num_edges = 0;
			{
				std::vector<std::tuple<vertex_id_type, vertex_id_type, edge_id_type>> edge_set;
				for(const auto& [vtx1, vtx2]: edges) {
					if(vtx1 == vtx2) {
						throw std::runtime_error("Loops are not allowed");
					}
					if(vtx1 < 0 || vtx2 < 0 || vtx1 >= num_vertices_ || vtx2 >= num_vertices_) {
						throw std::runtime_error("vertex indices out of range");
					}

					edge_set.push_back({vtx1, vtx2, num_edges});
					edge_set.push_back({vtx2, vtx1, num_edges});
					++num_edges;
				}
				std::sort(edge_set.begin(), edge_set.end());

				source_vtx_storage.resize(edge_set.size());
				dest_vtx_storage.resize(edge_set.size());
				edge_idx_storage.resize(edge_set.size());
				for(int i=0; i<edge_set.size(); i++) {
					const auto& [u, v, idx] = edge_set[i];
					source_vtx_storage[i] = u;
					dest_vtx_storage[i] = v;
					edge_idx_storage[i] = idx;
				}
			}


			offset_of_edge.resize(num_edges);
			for(int i=edge_idx_storage.size()-1; i>=0; i--) {
				offset_of_edge[edge_idx_storage[i]] = i;
			}
			

			adjacency_regions.resize(this->num_vertices_ + 1);
			adjacency_regions.front() = 0;
			adjacency_regions.back() = source_vtx_storage.size();
			for(int v=0; v<num_vertices_; v++) {
				if(v > 0) {
					adjacency_regions[v] = adjacency_regions[v-1];
				}
				auto& i = adjacency_regions[v];
				while(i < source_vtx_storage.size() && source_vtx_storage[i] < v) {
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
		
		[[nodiscard]] bool is_adjacent(int u, int v) const {
			auto [beg, end] = get_adjacency_range(u);
			return std::binary_search(&dest_vtx_storage[beg], &dest_vtx_storage[end], v);
		}

		[[nodiscard]] auto adjacent_vertices(int u) const {
			auto [beg, end] = get_adjacency_range(u);
			return std::span {dest_vtx_storage.data()+beg, dest_vtx_storage.data()+end};
		}

		[[nodiscard]] auto edge_at(int index) const {
			auto idx = offset_of_edge[index];
			return edge_view_type {source_vtx_storage[idx], dest_vtx_storage[idx], index};
		}

		[[nodiscard]] auto outgoing_edges(int u) const {
			auto [beg, end] = get_adjacency_range(u);

			return std::views::iota(beg, end) | std::views::transform([&](auto&& idx) {
				return edge_view_type {source_vtx_storage[idx], dest_vtx_storage[idx], edge_idx_storage[idx]};
			});
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}
		
		[[nodiscard]] auto all_edges() const {
			return offset_of_edge | std::views::transform([this](auto&& off) {
				return edge_view_type {source_vtx_storage[off], dest_vtx_storage[off], edge_idx_storage[off]};
			});
		}
		
		template<typename T>
		[[nodiscard]] auto create_vertex_label_container(T value = {}) const {
			return std::vector<T>(num_vertices(), value);
		}
		
		template<typename T>
		[[nodiscard]] auto create_edge_label_container(T value = {}) const {
			return std::vector<T>(num_edges(), value);
		}
	};
	static_assert(graph<static_simple_graph>);
	
}

#endif //GRAPH2X_STATIC_SIMPLE_GRAPH_HPP
