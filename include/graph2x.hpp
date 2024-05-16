
#ifndef GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
#define GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85

#include <concepts>
#include <vector>
#include <span>
#include <ranges>
#include <unordered_set>
#include <algorithm>
#include <assert.h>
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

			explicit rust_like_range(TNextFn&& fn) : fn_(std::forward<TNextFn>(fn)) {

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

		struct always_true {
			template<typename... Ts>
			bool operator()(Ts&&...) {
				return true;
			}
		};

		struct always_false {
			template<typename... Ts>
			bool operator()(Ts&&...) {
				return true;
			}
		};
	}


	class static_simple_graph {
	public:
		using vertex_id_type = int;
		using edge_id_type = int;
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
		//TODO fix copying!!!!
		std::vector<std::span<edge_type>> adjacency_partitions;

		//i-th element is the index to edge_storage to one of the two edges that satisfy i==idx
		std::vector<int> offset_of_edge;

	public:


		bool is_directed = false;

		static_simple_graph(int num_vertices, std::ranges::forward_range auto&& edges) {
			this->num_vertices_ = num_vertices;

			std::set<edge_type> edge_set;
			int num_edges = 0;
			for(const auto& [vtx1, vtx2]: edges) {
				if(vtx1 == vtx2) {
					throw std::runtime_error("loops are not allowed");
				}
				int index_of_edge = num_edges++;

				int u = std::min(vtx1, vtx2);
				int v = std::max(vtx1, vtx2);
				edge_set.emplace(u, v, index_of_edge);
				edge_set.emplace(v, u, index_of_edge);
			}

			edge_storage = {edge_set.begin(), edge_set.end()};

			offset_of_edge.resize(num_edges);
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
			return offset_of_edge.size();
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

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto all_edges() const {
			return offset_of_edge | std::views::transform([this](auto&& off) {
				return edge_storage[off];
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

	template<typename GraphT>
	auto edge_at(GraphT&& graph, auto&& index) {
		return graph.edge_at(index);
	}

	template<typename GraphT, typename T>
	auto create_vertex_label_container(GraphT&& graph, T value = {}) {
		return graph.template create_vertex_label_container<T>(value);
	}

	template<typename GraphT, typename T>
	auto create_edge_label_container(GraphT&& graph, T value = {}) {
		return graph.template create_edge_label_container<T>(value);
	}

	template<typename GraphT>
	using vertex_id_t = typename std::remove_cvref_t<GraphT>::vertex_id_type;

	template<typename GraphT>
	using edge_id_t = typename std::remove_cvref_t<GraphT>::edge_id_type;

	template<typename GraphT>
	using edge_t = typename std::remove_cvref_t<GraphT>::edge_type;

	enum class vertex_search_state {
		unvisited,
		marked,
		visited
	};


	namespace algo {

		template<typename GraphT>
		class bfs_queue {
		public:
			using graph_type = GraphT;
			using vertex_id_type = vertex_id_t<GraphT>;
			using edge_id_type = edge_id_t<GraphT>;

			explicit bfs_queue(int num_vertices): num_vertices(num_vertices) {

			}

			void expect_full_search() {
				storage.reserve(num_vertices);
			}

			void push(const vertex_id_type& v) {
				storage.push_back(v);
			}

			vertex_id_type pop() {
				if(is_empty()) {
					throw std::range_error("cannot pop from an empty queue");
				}
				auto r = storage.at(tail++);
				return r;
			}

			[[nodiscard]] bool is_empty() const {
				return storage.size() <= tail;
			}
		private:
			std::vector<vertex_id_type> storage;
			int num_vertices;
			size_t tail = 0;
		};

		template<typename GraphT>
		class dfs_stack {

		public:
			using graph_type = GraphT;
			using vertex_id_type = vertex_id_t<GraphT>;
			using edge_id_type = edge_id_t<GraphT>;

			explicit dfs_stack(int num_vertices) {

			}

			void expect_full_search() {
				storage.reserve(num_vertices);
			}

			void push(const vertex_id_type& v) {
				storage.push_back(v);
			}
			vertex_id_type pop() {
				if(is_empty()) {
					throw std::range_error("cannot pop from an empty queue");
				}
				auto r = storage.back();
				storage.pop_back();
				return r;
			}
			[[nodiscard]] bool is_empty() const {
				return storage.empty();
			}
		private:
			int num_vertices;
			std::vector<vertex_id_type> storage;
		};

		template<typename GraphT, template<class> class SearcherTTP, typename EdgePredicateRefT = const detail::always_true&>
		class generic_graph_search {
		public:
			using graph_type = GraphT;
			using vertex_id_type = vertex_id_t<GraphT>;
			using edge_id_type = edge_id_t<GraphT>;
			using edge_type = edge_t<GraphT>;

			explicit generic_graph_search(const GraphT& graph, EdgePredicateRefT&& edge_predicate = edge_predicate_type{})
				: graph(graph),
					searcher(num_vertices(graph)),
					edge_container(create_vertex_label_container(graph, edge_opt{})),
					state_container(create_vertex_label_container(graph, vertex_search_state::unvisited)),
					edge_predicate(std::forward<EdgePredicateRefT>(edge_predicate))
			{}

			generic_graph_search(generic_graph_search&&) = default;
			generic_graph_search& operator=(generic_graph_search&&) = default;


			void add_vertex(const vertex_id_type& v) {
				if(get_vertex_state(v) != vertex_search_state::unvisited) {
					throw std::runtime_error("cannot add a marked/visited vertex for searching");
				}
				searcher.push(v);
				state_container[v] = vertex_search_state::marked;
			}

			std::optional<vertex_id_type> next_vertex() {
				if(is_finished()) {
					return std::nullopt;
				}
				auto vtx = searcher.pop();
				state_container[vtx] = vertex_search_state::visited;
				for(auto& edge: outgoing_edges(graph, vtx)) {
					const auto& [u, v, i] = edge;
					if(edge_predicate(edge) && get_vertex_state(v) == vertex_search_state::unvisited) {
						add_vertex(v);
						edge_container[v] = edge;
					}
				}
				return vtx;
			}

			[[nodiscard]] vertex_search_state get_vertex_state(const vertex_id_type& v) const {
				return state_container[v];
			}

			std::optional<edge_type> next_edge() {
				while(auto v_opt = next_vertex()) {
					if(edge_container[*v_opt].has_value()) {
						return *edge_container[*v_opt];
					}
				}
				return std::nullopt;
			}

			template<typename ContainerT>
			void update_distances(const vertex_id_type& vtx, ContainerT&& vtx_label_container) {
				if(auto opt_edge = source_edge(vtx)) {
					const auto& [u, v, i] = *opt_edge;
					vtx_label_container[v] = vtx_label_container[u] + 1;
				} else {
					vtx_label_container[vtx] = 0;
				}
			}

			std::optional<edge_type> source_edge(const vertex_id_type& v) {
				return edge_container[v];
			}

			[[nodiscard]] bool is_finished() const {
				return searcher.is_empty();
			}

		private:

			using edge_opt = std::optional<edge_type>;
			using edge_container_type = decltype(create_vertex_label_container<GraphT, edge_opt>(std::declval<GraphT>()));
			using state_container_type = decltype(create_vertex_label_container<GraphT, vertex_search_state>(std::declval<GraphT>()));
			using edge_predicate_type = std::remove_cvref_t<EdgePredicateRefT>;

			const GraphT& graph;

			SearcherTTP<GraphT> searcher;

			edge_container_type edge_container;
			state_container_type state_container;

			edge_predicate_type edge_predicate;
		};

		template<typename GraphT, typename EdgePredicateRefT = const detail::always_true&>
		using breadth_first_search = generic_graph_search<GraphT, bfs_queue, EdgePredicateRefT>;

		template<typename GraphT, typename EdgePredicateRefT = const detail::always_true&>
		using depth_first_search = generic_graph_search<GraphT, dfs_stack, EdgePredicateRefT>;


		template<typename SearchT, typename IdxOrRangeT>
		void generic_init_search(SearchT&& search, IdxOrRangeT&& start) {
			if constexpr(std::ranges::forward_range<std::remove_cvref_t<IdxOrRangeT>>) {
				for(auto& v: start) {
					search.add_vertex(v);
				}
			} else {
				search.add_vertex(start);
			}
		}


		template<typename GraphT, typename IdxOrRangeT>
		auto simple_edges_bfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			using graph_type = std::remove_cvref_t<GraphT>;
			breadth_first_search<graph_type> bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_edge();
			});
		}

		template<typename GraphT, typename IdxOrRangeT>
		auto simple_vertices_bfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			using graph_type = std::remove_cvref_t<GraphT>;
			breadth_first_search bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_vertex();
			});
		}

		template<typename GraphT, typename IdxOrRangeT>
		auto simple_edges_dfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			using graph_type = std::remove_cvref_t<GraphT>;
			depth_first_search<graph_type> dfs(graph);
			generic_init_search(dfs, start);
			return detail::rust_like_range([dfs = std::move(dfs)]() mutable {
				return dfs.next_edge();
			});
		}

		template<typename GraphT, typename IdxOrRangeT>
		auto simple_vertices_dfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			using graph_type = std::remove_cvref_t<GraphT>;
			depth_first_search dfs(graph);
			generic_init_search(dfs, start);
			return detail::rust_like_range([dfs = std::move(dfs)]() mutable {
				return dfs.next_vertex();
			});
		}


		template<typename GraphT>
		auto bipartite_decompose(GraphT&& graph) {

			auto labels = create_vertex_label_container<GraphT, char>(graph, -1);

			using result_type = std::optional<decltype(labels)>;
			breadth_first_search bfs(graph);

			for(const auto& vtx: all_vertices(graph)) {
				if(bfs.get_vertex_state(vtx) == vertex_search_state::unvisited) {
					labels[vtx] = 0;
					bfs.add_vertex(vtx);
				}
				while(auto opt_vtx = bfs.next_vertex()) {
					const auto& u = *opt_vtx;
					for(const auto& v: adjacent_vertices(graph, u)) {
						assert(labels[u] >= 0);
						if(labels[v] >= 0 && labels[v] == labels[u]) {
							return result_type{std::nullopt}; //odd cycle detected - graph is not bipartite
						}
						labels[v] = !labels[u];
					}
				}
			}

			return result_type{labels};
		}


		namespace detail {


			/* Runs the BFS stage of the Hopcroft-Karp algorithm and returns a vertex-label-container of
			 * BFS distances. In particular, unmatched left vertices have a distance of 0.
			 */
			template<typename GraphT>
			auto hopcroft_karp_bfs_stage(GraphT&& graph, auto&& partitions, auto&& matching) {
				auto edge_predicate = [&](auto&& edge) {
					const auto& [u, v, i] = edge;
					if(matching[i]) {
						return partitions[u] == 1 && partitions[v] == 0;
					} else {
						return partitions[u] == 0 && partitions[v] == 1;
					}
				};

				breadth_first_search bfs {graph, edge_predicate};


				auto vtx_matched = create_vertex_label_container(graph, char(false));
				auto is_endpoint_candidate = create_vertex_label_container(graph, char(false));

				for(const auto& [u, v, i]: all_edges(graph)) {
					if(matching[i]) {
						vtx_matched[u] = true;
						vtx_matched[v] = true;
					}
				}

				for(const auto& vtx: all_vertices(graph)) {
					if(partitions[vtx] == 0 && vtx_matched[vtx] == false) {
						bfs.add_vertex(vtx);
					}
				}

				int aug_path_length = std::numeric_limits<int>::max();

				auto bfs_layer = create_vertex_label_container(graph, -1);
				while(auto v_opt = bfs.next_vertex()) {
					auto v = *v_opt;
					bfs.update_distances(v, bfs_layer);

					if(bfs_layer[v] > aug_path_length) { //search is past all shortest augmenting paths and should terminate
						break;
					}

					if(partitions[v] == 1 && vtx_matched[v] == false) { //augmenting path endpoint candidate found
						is_endpoint_candidate[v] = true;
						aug_path_length = std::min(aug_path_length, bfs_layer[v]);
					}
				}

				for(const auto& v: all_vertices(graph)) {
					if(bfs_layer[v] == aug_path_length && not is_endpoint_candidate[v]) {
						bfs_layer[v] = -1;
					}
				}

				return bfs_layer;
			}

			template<typename GraphT>
			bool hopcroft_karp_dfs_step(
				GraphT&& graph,
				const auto& bfs_levels,
				const auto& endpoint_candidates,
				const auto& used_vertices,
				auto&& start_vertex,
				auto&& output_vertices)
			{
				*output_vertices++ = start_vertex;

				if(endpoint_candidates[start_vertex]) {
					return true;
				}

				for(const auto& [u, v, i]: outgoing_edges(graph, start_vertex)) {
					if(not used_vertices[v] && bfs_levels[u]+1 == bfs_levels[v]) {
						if(hopcroft_karp_dfs_step(graph, bfs_levels, endpoint_candidates, used_vertices, v, output_vertices)) {
							return true;
						}
					}
				}

				return false;

			}

			template<typename GraphT>
			auto hopcroft_karp_dfs_stage(
				GraphT&& graph,
				const auto& bfs_levels,
				const auto& start_vertices,
				const auto& endpoint_candidates,
				auto&& output_augmenting_set)
			{
				std::vector<edge_id_t<GraphT>> augpath;
				auto used_vertices = create_vertex_label_container(graph, char(false));

				for(const auto& start_vtx: start_vertices) {
					if(hopcroft_karp_dfs_step(graph, bfs_levels, endpoint_candidates, used_vertices, start_vtx, std::back_inserter(augpath))) {
						for(const auto& [u, v, i]: augpath) {
							used_vertices[u] = true;
							used_vertices[v] = true;
							*output_augmenting_set++ = i;
						}
					}

					augpath.clear();
				}
			}

		}

		template<typename GraphT>
		auto find_bipartite_augmenting_set(GraphT&& graph, auto&& partitions, auto&& matching) {

			auto edge_predicate = [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				if(matching[i]) {
					return partitions[u] == 1 && partitions[v] == 0;
				} else {
					return partitions[u] == 0 && partitions[v] == 1;
				}
			};

			breadth_first_search bfs {graph, edge_predicate};


			auto vtx_matched = create_vertex_label_container(graph, char(false));
			auto is_endpoint_candidate = create_vertex_label_container(graph, char(false));

			for(const auto& [u, v, i]: all_edges(graph)) {
				if(matching[i]) {
					vtx_matched[u] = true;
					vtx_matched[v] = true;
				}
			}

			for(const auto& vtx: all_vertices(graph)) {
				if(partitions[vtx] == 0 && vtx_matched[vtx] == false) {
					bfs.add_vertex(vtx);
				}
			}

			int aug_path_length = std::numeric_limits<int>::max();

			auto bfs_layer = create_vertex_label_container(graph, -1);
			while(auto v_opt = bfs.next_vertex()) {
				auto v = *v_opt;
				bfs.update_distances(v, bfs_layer);

				if(bfs_layer[v] > aug_path_length) { //search is past all shortest augmenting paths and should terminate
					break;
				}

				if(partitions[v] == 1 && vtx_matched[v] == false) { //augmenting path endpoint candidate found
					is_endpoint_candidate[v] = true;
					aug_path_length = std::min(aug_path_length, bfs_layer[v]);
				}
			}

			auto result = std::vector<edge_id_t<GraphT>>{};

			auto vtx_used = create_vertex_label_container(graph, char(false));
			auto edge_predicate_dfs = [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				return edge_predicate(edge) && !vtx_used && (bfs_layer[u]+1 == bfs_layer[v]);
			};
			depth_first_search dfs {graph, edge_predicate_dfs};
			for(const auto& vtx: all_vertices(graph)) {
				if(partitions[vtx] == 0 && vtx_matched[vtx] == false) {
					dfs.add_vertex(vtx);
				}
			}

			int dbg_num_augmenting_paths = 0;

			while(auto v_opt = dfs.next_vertex()) {
				const auto& v = *v_opt;
				if(is_endpoint_candidate[v]) { //augmenting path found - trace to source
					++dbg_num_augmenting_paths;
					auto cv = v;
					while(auto opt_edge = dfs.source_edge(cv)) {
						const auto& [u1, v1, i] = *opt_edge;
						cv = u1;
						result.push_back(i);
					}
				}
			}
			if(dbg_num_augmenting_paths) {
				printf("%d augmenting paths of length %d found\n", dbg_num_augmenting_paths, aug_path_length);
			}

			return result;

		}

		template<typename GraphT>
		auto max_bipartite_matching(GraphT&& graph) {

			auto partitions = bipartite_decompose(graph).value();
			auto matching = create_edge_label_container(graph, char(false));

			while(true) {
				auto aug_set = find_bipartite_augmenting_set(graph, partitions, matching);
				if(aug_set.empty()) {
					break;
				}
				for(const auto& idx: aug_set) {
					matching[idx] = !matching[idx];
				}
			}

			return matching;
		}

		template<typename GraphT>
		auto max_weight_bipartite_matching(GraphT&& graph, auto&& weights) {

		}

		template<typename GraphT>
		auto cycle_cover(GraphT&& graph) {

		}

	}


}


#endif //GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
