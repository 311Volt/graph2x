
#ifndef GRAPH2X_SEARCH_HPP_E65AC38138DD4840A7A8E07996A89616
#define GRAPH2X_SEARCH_HPP_E65AC38138DD4840A7A8E07996A89616

#include <vector>
#include "../core.hpp"
#include "../util.hpp"
#include <ranges>

namespace g2x {
	
	
	enum class vertex_search_state {
		unvisited,
		marked,
		visited
	};
	
	namespace algo {
		
		template<graph GraphT>
		class bfs_queue {
		public:
			using vertex_type = vertex_id_t<GraphT>;

			void push(const vertex_type& v) {
				storage.push_back(v);
			}
			
			vertex_type pop() {
				if(num_pending_items() <= 0) {
					throw std::range_error("cannot pop from an empty queue");
				}
				auto r = storage.at(tail++);
				return r;
			}

			void expect_up_to(size_t num_items) {
				storage.reserve(num_items);
			}
			
			[[nodiscard]] size_t num_pending_items() const {
				return storage.size() - tail;
			}

			auto pending_items() {
				return std::span<const vertex_type> {
					storage.data() + tail,
					storage.data() + storage.size()
				};
			}

			auto processed_items() {
				return std::span<const vertex_type> {
					storage.data(),
					storage.data() + tail
				};
			}

			void reset() {
				storage.resize(0);
				tail = 0;
			}
		private:
			std::vector<vertex_type> storage;
			size_t tail = 0;
		};
		
		template<typename GraphT>
		class dfs_stack {
		public:

			using vertex_type = vertex_id_t<GraphT>;



			void push(const vertex_type& v) {
				storage_stack.push_back(v);
			}

			vertex_type pop() {
				if(num_pending_items() <= 0) {
					throw std::range_error("cannot pop from an empty queue");
				}
				auto r = storage_stack.back();
				storage_stack.pop_back();
				storage_popped.push_back(r);
				return r;
			}

			void expect_up_to(size_t num_items) {
				storage_stack.reserve(num_items);
				storage_popped.reserve(num_items);
			}

			[[nodiscard]] size_t num_pending_items() const {
				return storage_stack.size();
			}

			auto pending_items() {
				return std::span<const vertex_type> {storage_stack};
			}

			auto processed_items() {
				return std::span<const vertex_type> {storage_popped};
			}

			void reset() {
				storage_stack.resize(0);
				storage_popped.resize(0);
			}
		private:
			std::vector<vertex_type> storage_stack;
			std::vector<vertex_type> storage_popped;
		};

		template<typename T>
		concept graph_search_structure = requires(T x, typename T::vertex_type v) {
			{x.push(v)};
			{x.pop()} -> std::convertible_to<typename T::vertex_type>;
			{x.expect_up_to(0)};
			{x.num_pending_items()} -> std::convertible_to<size_t>;
			{x.pending_items()} -> std::ranges::range;
			{x.processed_items()} -> std::ranges::range;
			{x.reset()};
		};
		
		template<
			graph GraphT,
			template<class> class SearchStructureTTP,
			typename EdgePredicateRefT = const detail::always_true&,
			typename VertexPredicateRefT = const detail::always_true&,
			typename AdjacencyProjectionRefT = const std::identity&
		>
			requires graph_search_structure<SearchStructureTTP<GraphT>>
		class generic_graph_search {
		public:
			using graph_type = GraphT;
			using vertex_id_type = vertex_id_t<GraphT>;
			using edge_id_type = edge_id_t<GraphT>;
			using edge_type = edge_t<GraphT>;
			
			explicit generic_graph_search(
				const GraphT& graph,
				EdgePredicateRefT&& edge_predicate = edge_predicate_type{},
				VertexPredicateRefT&& vertex_predicate = vertex_predicate_type{},
				AdjacencyProjectionRefT&& adjacency_projection = adjacency_projection_type{}
			)
				: graph_(graph),
				  search_structure_(),
				  state_container_(create_vertex_labeling(graph, vertex_search_state::unvisited)),
				  source_edge_container_(create_vertex_labeling<edge_opt>(graph)),
				  edge_predicate_(std::forward<EdgePredicateRefT>(edge_predicate)),
				  vertex_predicate_(std::forward<VertexPredicateRefT>(vertex_predicate)),
				  adjacency_projection_(std::forward<AdjacencyProjectionRefT>(adjacency_projection))
			{}
			
			generic_graph_search(generic_graph_search&&) = default;
			generic_graph_search& operator=(generic_graph_search&&) = default;


			
			void add_vertex(const vertex_id_type& v) {
				if(get_vertex_state(v) != vertex_search_state::unvisited) {
					throw std::runtime_error("cannot add a marked/visited vertex for searching");
				}
				search_structure_.push(v);
				state_container_.at(v) = vertex_search_state::marked;
			}
			
			std::optional<vertex_id_type> next_vertex() {
				if(is_finished()) {
					return std::nullopt;
				}
				auto vtx = search_structure_.pop();
				state_container_[vtx] = vertex_search_state::visited;
				auto&& outgoing_edges_view = outgoing_edges(this->graph_, vtx);
				for(const auto& edge: adjacency_projection_(outgoing_edges_view)) {
					const auto& [u, v, i] = edge;
					if(    get_vertex_state(v) == vertex_search_state::unvisited
						&& edge_predicate_(edge)
						&& vertex_predicate_(v)
					) {
						add_vertex(v);
						source_edge_container_[v] = edge;
					}
				}
				return vtx;
			}
			
			[[nodiscard]] vertex_search_state get_vertex_state(const vertex_id_type& v) const {
				if constexpr(requires{state_container_[v];}) {
					return state_container_[v];
				} else {
					return state_container_.at(v);
				}
			}
			
			std::optional<edge_type> next_edge() {
				while(auto v_opt = next_vertex()) {
					if(source_edge(*v_opt).has_value()) {
						return source_edge(*v_opt);
					}
				}
				return std::nullopt;
			}

			void update_distances(
				const vertex_id_type& vtx,
				vertex_labeling_of<GraphT, int> auto&& vtx_labeling
			) {
				if(auto opt_edge = source_edge(vtx)) {
					const auto& [u, v, i] = *opt_edge;
					vtx_labeling[v] = vtx_labeling[u] + 1;
				} else {
					vtx_labeling[vtx] = 0;
				}
			}
			
			std::optional<edge_type> source_edge(const vertex_id_type& v) {
				return source_edge_container_[v];
			}
			
			[[nodiscard]] bool is_finished() const {
				return search_structure_.num_pending_items() == 0;
			}

			void trace_path(const vertex_id_type& vtx, auto&& out_edge_ids) {
				auto cur_vtx = vtx;
				while(auto src_edge_opt = source_edge(cur_vtx)) {
					const auto& [u, v, i] = *src_edge_opt;
					cur_vtx = u;
					*out_edge_ids++ = i;
				}
			}

			void reset() {

				int full_reset_threshold = num_vertices(graph_) / 4;
				if(std::ranges::size(search_structure_.processed_items()) > full_reset_threshold) {
					std::ranges::fill(state_container_, vertex_search_state::unvisited);
					std::ranges::fill(source_edge_container_, edge_opt{});
				} else {
					for(const auto& v: search_structure_.pending_items()) {
						state_container_[v] = vertex_search_state::unvisited;
					}
					for(const auto& v: search_structure_.processed_items()) {
						state_container_[v] = vertex_search_state::unvisited;
						source_edge_container_[v] = std::nullopt;
					}
				}

				search_structure_.reset();
			}

			void expect_up_to(int num_vertices) {
				search_structure_.expect_up_to(num_vertices);
			}
		
		private:
			
			using edge_opt = std::optional<edge_type>;
			using edge_container_type = decltype(create_vertex_labeling<edge_opt, GraphT>(std::declval<GraphT>()));
			using state_container_type = decltype(create_vertex_labeling<vertex_search_state, GraphT>(std::declval<GraphT>()));

			using edge_predicate_type = std::remove_cvref_t<EdgePredicateRefT>;
			using vertex_predicate_type = std::remove_cvref_t<VertexPredicateRefT>;
			using adjacency_projection_type = std::remove_cvref_t<AdjacencyProjectionRefT>;
			
			const GraphT& graph_;
			
			SearchStructureTTP<GraphT> search_structure_;

			edge_container_type source_edge_container_;
			state_container_type state_container_;

			edge_predicate_type edge_predicate_;
			vertex_predicate_type vertex_predicate_;
			adjacency_projection_type adjacency_projection_;
		};
		
		template<
			typename GraphT,
			typename EdgePredicateRefT = const detail::always_true&,
			typename VertexPredicateRefT = const detail::always_true&,
			typename AdjacencyProjectionRefT = const std::identity&
		>
		using breadth_first_search = generic_graph_search<GraphT, bfs_queue, EdgePredicateRefT, VertexPredicateRefT, AdjacencyProjectionRefT>;
		
		template<
			typename GraphT,
			typename EdgePredicateRefT = const detail::always_true&,
			typename VertexPredicateRefT = const detail::always_true&,
			typename AdjacencyProjectionRefT = const std::identity&
		>
		using depth_first_search = generic_graph_search<GraphT, dfs_stack, EdgePredicateRefT, VertexPredicateRefT, AdjacencyProjectionRefT>;
		
		
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
		
		
		template<graph GraphT, typename IdxOrRangeT>
		auto simple_edges_bfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			breadth_first_search bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_edge();
			});
		}
		
		template<graph GraphT, typename IdxOrRangeT>
		auto simple_vertices_bfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			breadth_first_search bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_vertex();
			});
		}
		
		template<graph GraphT, typename IdxOrRangeT>
		auto simple_edges_dfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			depth_first_search dfs(graph);
			generic_init_search(dfs, start);
			return detail::rust_like_range([dfs = std::move(dfs)]() mutable {
				return dfs.next_edge();
			});
		}
		
		template<graph GraphT, typename IdxOrRangeT>
		auto simple_vertices_dfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			depth_first_search dfs(graph);
			generic_init_search(dfs, start);
			return detail::rust_like_range([dfs = std::move(dfs)]() mutable {
				return dfs.next_vertex();
			});
		}
		
	}
	
}


#endif //GRAPH2X_SEARCH_HPP_E65AC38138DD4840A7A8E07996A89616
