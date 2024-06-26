
#ifndef GRAPH2X_SEARCH_HPP_E65AC38138DD4840A7A8E07996A89616
#define GRAPH2X_SEARCH_HPP_E65AC38138DD4840A7A8E07996A89616

#include <vector>
#include "../core.hpp"

namespace g2x {
	
	
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
				state_container.at(v) = vertex_search_state::marked;
			}
			
			std::optional<vertex_id_type> next_vertex() {
				if(is_finished()) {
					return std::nullopt;
				}
				auto vtx = searcher.pop();
				state_container[vtx] = vertex_search_state::visited;
				for(const auto& edge: outgoing_edges(graph, vtx)) {
					const auto& [u, v, i] = edge;
					if(edge_predicate(edge) && get_vertex_state(v) == vertex_search_state::unvisited) {
						add_vertex(v);
						edge_container.at(v) = edge;
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

			void trace_path(const vertex_id_type& vtx, auto&& out_edge_ids) {
				auto cur_vtx = vtx;
				while(auto src_edge_opt = source_edge(cur_vtx)) {
					const auto& [u, v, i] = *src_edge_opt;
					cur_vtx = u;
					*out_edge_ids++ = i;
				}
			}
		
		private:
			
			using edge_opt = std::optional<edge_type>;
			using edge_container_type = decltype(create_vertex_label_container<edge_opt, GraphT>(std::declval<GraphT>()));
			using state_container_type = decltype(create_vertex_label_container<vertex_search_state, GraphT>(std::declval<GraphT>()));
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
			breadth_first_search bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_edge();
			});
		}
		
		template<typename GraphT, typename IdxOrRangeT>
		auto simple_vertices_bfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			breadth_first_search bfs(graph);
			generic_init_search(bfs, start);
			return detail::rust_like_range([bfs = std::move(bfs)]() mutable {
				return bfs.next_vertex();
			});
		}
		
		template<typename GraphT, typename IdxOrRangeT>
		auto simple_edges_dfs(const GraphT& graph, IdxOrRangeT&& start = {}) {
			depth_first_search dfs(graph);
			generic_init_search(dfs, start);
			return detail::rust_like_range([dfs = std::move(dfs)]() mutable {
				return dfs.next_edge();
			});
		}
		
		template<typename GraphT, typename IdxOrRangeT>
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
