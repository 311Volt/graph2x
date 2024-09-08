
#ifndef GRAPH2X_STATIC_SIMPLE_GRAPH_HPP
#define GRAPH2X_STATIC_SIMPLE_GRAPH_HPP

#include <algorithm>
#include <vector>
#include <span>
#include <set>

#include "../core.hpp"


namespace g2x {
	
	/*
	 * An immutable graph.
	 *
	 * Creation: O(V + E*log(E))
	 * Adjacency check: O(log(N(v)))
	 * Pass over outgoing_edges: O(N(v))
	 * Pass over adjacent_vertices: O(N(v))
	 * Pass over all_vertices: O(V)
	 * Pass over all_edges: O(E)
	 * Edge index lookup: O(1)
	 */

	template<std::integral VIdxT, std::integral EIdxT = int, bool IsDirected = false>
	class general_basic_graph {
	public:
		using vertex_id_type = VIdxT;
		using edge_id_type = EIdxT;
		using edge_offset_type = edge_id_type;
		using edge_value_type = edge_value<vertex_id_type, edge_id_type>;

		static constexpr bool is_directed = IsDirected;
		static constexpr bool natural_vertex_numbering = true;
		static constexpr bool natural_edge_numbering = true;
	private:
		isize num_vertices_;

		std::vector<edge_value_type> edge_storage;

		//the i-th and i+1-th elements denote the index range of adjacency_storage
		std::vector<edge_offset_type> adjacency_regions;

		//i-th element is the index into edge_index_storage to one of the two edges that satisfy i==idx
		std::vector<edge_offset_type> offset_of_edge;

		[[nodiscard]] std::pair<edge_offset_type, edge_offset_type> get_adjacency_range(vertex_id_type v) const {
			return {adjacency_regions.at(v), adjacency_regions.at(v+1)};
		}

		[[nodiscard]] bool is_edge_unique(const edge_value_type& e) const {
			if constexpr(is_directed) {
				return true;
			}
			return e.u <= e.v;
		}
	
	public:
		
		general_basic_graph(vertex_count num_vertices, std::ranges::forward_range auto&& edges) {

			isize counted_num_vertices = 0;

			edge_id_type num_edges = 0;

			if constexpr(std::ranges::sized_range<std::remove_cvref_t<decltype(edges)>>) {
				edge_storage.reserve(std::ranges::size(edges));
			}

			for(const auto& [vtx1, vtx2]: edges) {
				if(num_edges == std::numeric_limits<decltype(num_edges)>::max()) {
					throw std::out_of_range(std::format("Limit of {} edges exceeded", num_edges));
				}

				bool idx_out_of_bound = false;
				if(vtx1 < 0 || vtx2 < 0) {
					idx_out_of_bound = true;
				}
				if(num_vertices.has_value() && (vtx1 >= num_vertices.value() || vtx2 >= num_vertices.value())) {
					idx_out_of_bound = true;
				}
				if(idx_out_of_bound) {
					throw std::out_of_range(std::format("vertex indices ({}, {}) out of range: [{}; {})", vtx1, vtx2, 0, num_vertices.value_or(counted_num_vertices)));
				}

				counted_num_vertices = std::max<isize>(counted_num_vertices, std::max(vtx1, vtx2));

				edge_storage.push_back({vtx1, vtx2, num_edges});
				if(not IsDirected && vtx1 != vtx2) {
					edge_storage.push_back({vtx2, vtx1, num_edges});
				}
				++num_edges;
			}
			std::sort(edge_storage.begin(), edge_storage.end(), [](auto&& e1, auto&& e2) {
				const auto& [u1, v1, i1] = e1;
				const auto& [u2, v2, i2] = e2;
				return std::make_tuple(u1, v1, i1) < std::make_tuple(u2, v2, i2);
			});

			this->num_vertices_ = num_vertices.value_or(counted_num_vertices);

			offset_of_edge.resize(num_edges);
			for(isize i=edge_storage.size()-1; i>=0; i--) {
				offset_of_edge[edge_storage[i].i] = i;
			}

			if(this->num_vertices_ >= std::numeric_limits<vertex_id_type>::max()) {
				throw std::out_of_range(std::format("Cannot create adjacency regions: limit of {} vertices exceeded", num_vertices_-1));
			}
			

			adjacency_regions.resize(this->num_vertices_ + 1);
			adjacency_regions.front() = 0;
			adjacency_regions.back() = edge_storage.size();
			for(isize v=0; v<num_vertices_; v++) {
				if(v > 0) {
					adjacency_regions[v] = adjacency_regions[v-1];
				}
				auto& i = adjacency_regions[v];
				while(i < edge_storage.size() && edge_storage[i].u < v) {
					i++;
				}
			}
			
			
		}
		
		[[nodiscard]] isize num_vertices() const {
			return num_vertices_;
		}
		
		[[nodiscard]] isize num_edges() const {
			return offset_of_edge.size();
		}

		[[nodiscard]] auto outgoing_edges(vertex_id_type u) const {
			auto [beg, end] = get_adjacency_range(u);

			return std::span{edge_storage.data() + beg, edge_storage.data() + end};
		}

		[[nodiscard]] auto edge_at(edge_id_type index) const {
			return edge_storage[offset_of_edge[index]];
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}
		
		[[nodiscard]] auto all_edges() const {
			return edge_storage | std::views::filter([this](const edge_value_type& e){return is_edge_unique(e);});
		}

	};

	template<typename VIdxT, typename EIdxT = int>
	using general_basic_digraph = general_basic_graph<VIdxT, EIdxT, true>;

	using basic_graph = general_basic_graph<int>;
	using basic_graph_16 = general_basic_graph<int16_t>;
	using basic_graph_8 = general_basic_graph<int8_t>;

	using basic_digraph = general_basic_digraph<int>;
	using basic_digraph_16 = general_basic_digraph<int16_t>;
	using basic_digraph_8 = general_basic_digraph<int8_t>;

	static_assert(graph<basic_graph>);
	
}

#endif //GRAPH2X_STATIC_SIMPLE_GRAPH_HPP
