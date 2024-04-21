
#ifndef GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
#define GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85

#include <concepts>
#include <vector>
#include <span>
#include <ranges>
#include <unordered_set>
#include <algorithm>
#include <deque>
#include <limits>
#include <set>
#include <unordered_map>

namespace g2x {

	/*
	 * graphs:
	 * - adjacency lists
	 *   - range of ranges
	 * - edge lists
	 *   - range of pairs
	 * - adjacency matrix
	 *
	 * graph free function interface:
	 * 	num_vertices(graph) -> int
	 * 	num_edges(graph) -> int
	 * 	is_adjacent(graph, v1, v2) -> bool
	 * 	adjacent_vertices(graph, v) -> sized_range<int>
	 * 	outgoing_edges(graph, v) -> sized_range<EdgeType>
	 * 	all_vertices(graph) -> range<int>
	 * 	all_edges(graph) -> range<EdgeType>
	 *
	 * vertex index: integral type
	 *
	 * edge free function interface:
	 *  first_vertex(edge) -> VIdxT
	 *  second_vertex(edge) -> VIdxT
	 *  [SFINAE - for directed only] source_vertex(edge) -> VIdxT
	 *  [SFINAE - for directed only] target_vertex(edge) -> VIdxT
	 *  edge_weight(edge) -> any
	 *
	 *
	 * graph mutation:
	 *  add_vertex(graph) -> vertex_idx
	 *  add_edge(graph, u, v, opt_weight) -> bool?
	 *  delete_vertex(graph)
	 *
	 * possible vertex index types:
	 *	- is_integral -> std::vector storage
	 *	- comparable -> std::map storage
	 *	- hashable -> std::unordered_map storage
	 *
	 * basic algorithms:
	 *  - breadth_first_search(graph, root=0) -> range<EdgeType>
	 *  - depth_first_search(graph, root=0) -> range<EdgeType>
	 *
	 * advanced algorithms:
	 *  - bipartite_decompose(graph, out_u, out_v)
	 *  - find_augmenting_path(graph, matching: rar[edges]) -> vector<EdgeIdxT>
	 *  - maximum_matching(graph, out_e)
	 *  - max_bipartite_matching(graph, out_e)
	 *  - max_weight_bipartite_matching(graph, out_e)
	 *
	 * graph creation:
	 * 	create_graph_from_edges<StorageClass>(<range of pairs>)
	 *	create_graph_from_adj_lists<StorageClass>(<range>)
	 *
	 * graph kinds:
	 *  - simple undirected
	 *  - simple directed
	 *  - weighted/unweighted
	 *  - single/multiple edge
	 *
	 * graph classes:
	 *  - static_sparse_graph
	 *  - static_dense_graph
	 *  - dynamic_sparse_graph
	 *  - dynamic_dense_graph
	 *
	 * graph member typedefs:
	 *  - vertex_id_type (integral/comparable/hashable)
	 *  - edge_id_type (integral/pair<vid,vid>)
	 *
	 * static/dynamic:
	 *  - static = is_integral<edge_index_type> && is_adjacent() at most log(E)
	 *  - dynamic = provides
	 */

	using vertex_idx = int64_t;

	namespace detail {

		template<typename TNextFn>
		class rust_like_range {
		private:
			using next_fn_type = std::remove_cvref_t<TNextFn>;
			using value_type_opt = std::invoke_result_t<next_fn_type>;
			using value_type = typename value_type_opt::value_type;

			next_fn_type fn_;
		public:

			explicit rust_like_range(const TNextFn& fn) : fn_(fn) {

			}

			class sentinel {};
			class iterator {
			public:
				next_fn_type& fn_;
				value_type_opt val_;

				bool operator==(const sentinel&) const {
					return not val_.has_value();
				}

				void operator++() {
					val_ = fn_();
				}

				value_type& operator*() {
					return *val_;
				}

				const value_type& operator*() const {
					return *val_;
				}
			};


			iterator begin() {
				auto result = iterator {.fn_ = fn_};
				++result;
				return result;
			}

			sentinel end() {
				return sentinel{};
			}
		};

	}


	class static_simple_graph {
	public:
		struct edge_type {
			int u;
			int v;
			int idx = -1;

			friend auto operator<=>(const edge_type& e1, const edge_type& e2) = default;
		};
	private:
		int num_vertices_;

		//edges, in lexicographical order, duplicated
		std::vector<edge_type> edge_storage;

		//subspans of edge_storage where the i-th element spans all edges adjacent to vertex v_i
		std::vector<std::span<edge_type>> adjacency_partitions;

		//i-th element is the index to edge_storage to one of the two edges that satisfy i==idx
		std::vector<int> offset_of_edge;

	public:

		bool is_directed = false;

		static_simple_graph(int num_vertices, std::ranges::forward_range auto&& edges) {
			this->num_vertices_ = num_vertices;

			std::set<edge_type> edge_set;
			for(int i=0; const auto& [vtx1, vtx2]: edges) {
				if(vtx1 == vtx2) {
					throw std::runtime_error("loops are not allowed");
				}
				int index_of_edge = i++;

				int u = std::min(vtx1, vtx2);
				int v = std::max(vtx1, vtx2);
				edge_set.emplace(u, v, index_of_edge);
				edge_set.emplace(v, u, index_of_edge);
			}

			edge_storage = {edge_set.begin(), edge_set.end()};

			offset_of_edge.resize(edge_storage.size());
			for(int off=0; off<edge_storage.size(); off++) {
				const auto& [u, v, i] = edge_storage[off];
				if(u < v) {
					offset_of_edge[i] = off;
				}
			}

			adjacency_partitions.resize(num_vertices);
			for(int v=0; v<num_vertices; v++) {
				auto begin = std::ranges::lower_bound(edge_storage, edge_type{v, 0});
				auto end = std::ranges::lower_bound(edge_storage, edge_type{v+1, 0});
				adjacency_partitions[v] = std::span{begin, end};
			}


		}

		[[nodiscard]] int num_vertices() const {
			return num_vertices_;
		}

		[[nodiscard]] int num_edges() const {
			return edge_storage.size();
		}

		[[nodiscard]] bool is_adjacent(int u, int v) const {
			const auto a1 = std::ranges::lower_bound(edge_storage, edge_type{u, v});
			const auto a2 = std::ranges::lower_bound(edge_storage, edge_type{u, v+1});

			return std::distance(a1, a2) != 0;
		}

		[[nodiscard]] auto outgoing_edges(int u) const {
			return adjacency_partitions.at(u);
		}

		[[nodiscard]] auto adjacent_vertices(int u) const {
			return outgoing_edges(u) | std::views::transform([](auto&& edge) {
				const auto& [u1, v1, i] = edge;
				return v1;
			});
		}

		[[nodiscard]] auto edge_at(int index) const {
			return edge_storage[offset_of_edge[index]];
		}

		[[nodiscard]] auto edge_idx_func() const {
			return [this](int idx) {
				return edge_at(idx);
			};
		}

		[[nodiscard]] auto all_vertices(int u) const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto all_edges(int u) const {
			return std::views::iota(0, num_edges()) | std::views::transform(edge_idx_func());
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

	template<typename GraphT>
	auto num_vertices(GraphT&& graph) {
		return graph.num_vertices();
	}

	template<typename GraphT>
	auto num_edges(GraphT&& graph) {
		return graph.num_edges();
	}

	template<typename GraphT>
	bool is_adjacent(GraphT&& graph, int u, int v) {
		return graph.is_adjacent(u, v);
	}

	template<typename GraphT>
	auto adjacent_vertices(GraphT&& graph, int v) {
		return graph.adjacent_vertices(v);
	}

	template<typename GraphT>
	auto outgoing_edges(GraphT&& graph, int v) {
		return graph.outgoing_edges(v);
	}

	template<typename GraphT>
	auto all_vertices(GraphT&& graph) {
		return graph.all_vertices();
	}

	template<typename GraphT>
	auto all_edges(GraphT&& graph) {
		return graph.all_edges();
	}

	namespace algo {

		template<typename GraphT, typename IdxOrRangeT>
		auto breadth_first_search_edges(GraphT&& graph, IdxOrRangeT&& start = {}) {

			using edge_type = typename std::remove_cvref_t<GraphT>::edge_type;

			int graph_num_vtxs = num_vertices(graph);
			std::vector<std::pair<int, std::optional<edge_type>>> initial_bfs_queue;
			if constexpr(std::ranges::forward_range<std::remove_cvref_t<IdxOrRangeT>>) {
				for(auto& v: start) {
					if(v < 0 || v >= graph_num_vtxs) {
						throw std::runtime_error("invalid start vertex: " + std::to_string(v));
					}
					initial_bfs_queue.emplace_back(v, std::nullopt);
				}
			} else {
				initial_bfs_queue.emplace_back(start, std::nullopt);
			}
			int start_size = initial_bfs_queue.size();
			initial_bfs_queue.resize(graph_num_vtxs);

			enum class vtx_color: char {
				unvisited,
				marked,
				visited
			};


			return detail::rust_like_range(
				[
					&graph,
					bfs_queue = std::vector{std::move(initial_bfs_queue)},
					bfs_queue_head = start_size,
					bfs_queue_tail = 0,
					color = std::vector<vtx_color>(graph_num_vtxs, vtx_color::unvisited)
				] mutable -> std::optional<typename std::remove_cvref_t<GraphT>::edge_type>
				{
				skip_source_vertex:
					if(bfs_queue_tail < bfs_queue_head) {
						auto [vtx, opt_edge] = bfs_queue.at(bfs_queue_tail++);
						printf("visiting %d\n", vtx);
						color[vtx] = vtx_color::visited;

						for(const auto& edge: outgoing_edges(graph, vtx)) {
							const auto& [u, v, i] = edge;
							if(color[v] == vtx_color::unvisited) {
								printf("adding %d [%d, %d, %d]\n", v, u, v, i);
								bfs_queue.at(bfs_queue_head++) = {v, edge};
								color[v] = vtx_color::marked;
							}
						}

						if(opt_edge.has_value()) {
							return *opt_edge;
						} else {
							goto skip_source_vertex;
						}

					} else {
						return std::nullopt;
					}
				}
			);
		}

		auto breadth_first_search_vertices() {
			throw std::runtime_error("unimplemented");
		}

		auto depth_first_search_edges() {
			throw std::runtime_error("unimplemented");
		}

		auto depth_first_search_vertices() {
			throw std::runtime_error("unimplemented");
		}

	}


}


#endif //GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
