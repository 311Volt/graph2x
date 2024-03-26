
#ifndef GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
#define GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85

#include <concepts>
#include <vector>
#include <span>
#include <ranges>
#include <unordered_set>
#include <algorithm>
#include <deque>

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
	 * graph mutation:
	 *  add_vertex(graph) -> vertex_idx
	 *  add_edge(graph, u, v) -> bool?
	 *
	 * basic algorithms:
	 *  breadth_first_search(graph, root=0) -> range<EdgeType>
	 *  depth_first_search(graph, root=0) -> range<EdgeType>
	 *
	 * graph creation:
	 * 	create_graph_from_edges<StorageClass>(<range of pairs>)
	 *	create_graph_from_adj_lists<StorageClass>(<range>)
	 *
	 * storage classes:
	 * 	
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


	template<typename TRange>
	concept graph_adjlist_range = requires {
		requires std::ranges::sized_range<TRange>;
		requires std::ranges::random_access_range<TRange>;
		requires std::ranges::forward_range<std::ranges::range_value_t<TRange>>;
		requires std::integral<std::ranges::range_value_t<std::ranges::range_value_t<TRange>>>;
	};

	template<typename TRange>
	concept graph_edge_range = requires(TRange r) {
		requires std::ranges::forward_range<TRange>;
		//TODO remaining constraints
	};


	class static_graph {
		std::vector<vertex_idx> storage;
		std::vector<std::span<vertex_idx>> outgoing_segments;
		std::vector<std::span<vertex_idx>> segments;
	public:

		struct directed_adjlist_tag_t {};
		struct directed_edgelist_tag_t {};
		struct undirected_adjlist_tag_t {};
		struct undirected_edgelist_tag_t {};

		[[nodiscard]] vertex_idx num_vertices() const {
			return segments.size();
		}

		[[nodiscard]] vertex_idx num_edges() const {
			return storage.size();
		}

		[[nodiscard]] bool is_adjacent(vertex_idx u, vertex_idx v) const {
			return std::ranges::end(segments) != std::ranges::lower_bound(segments.at(u), v);
		}

		[[nodiscard]] auto adjacent_vertices(vertex_idx idx) const {
			return segments.at(idx);
		}

		[[nodiscard]] auto outgoing_edges(vertex_idx idx) const {
			return outgoing_segments[idx] | std::views::transform([idx](vertex_idx dst) {
				return std::pair{idx, dst};
			});
		}

		[[nodiscard]] auto all_vertices() const {
			return std::views::iota(0, num_vertices());
		}

		[[nodiscard]] auto all_edges() const {
			return detail::rust_like_range([this, seg=vertex_idx(0), idx=vertex_idx(0)]() mutable -> std::optional<std::pair<vertex_idx, vertex_idx>> {
				if(seg >= outgoing_segments.size()) {
					return std::nullopt;
				}
				auto edge = std::pair{seg, outgoing_segments[seg][idx]};
				if(idx >= outgoing_segments[seg].size()) {
					++seg;
					idx = 0;
				}
				return edge;
			});
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
		return graph.adjacent_vertices(v);
	}

	template<typename GraphT>
	auto all_vertices(GraphT&& graph) {
		return graph.all_vertices();
	}

	template<typename GraphT>
	auto all_edges(GraphT&& graph) {
		return graph.all_edges();
	}


	inline namespace algo {

		template<typename TGraph>
		auto breadth_first_search(TGraph&& graph, vertex_idx root = 0) {
			return detail::rust_like_range(
				[
					&graph,
					bfs_queue = std::deque<int>(1, root),
					source = std::vector<int>(num_vertices(graph), -1),
					visited = std::vector<char>(num_vertices(graph), 0)
				]() mutable {
					using result_type = std::optional<std::pair<int, int>>;
					if(bfs_queue.empty()) {
						return std::nullopt;
					}
					int curr = bfs_queue.front();
					bfs_queue.pop_front();

					for(const auto& v: adjacent_vertices(graph, curr)) {
						if(not visited[v]) {
							bfs_queue.push_back(v);
							source[v] = curr;
						}
					}
					visited[curr] = 1;
					return std::pair {source[curr], curr};
				}
			);
		}

		template<typename TGraph>
		auto depth_first_search(TGraph&& graph, vertex_idx root = 0) {
			return detail::rust_like_range(
				[
					&graph,
					dfs_stack = std::vector<int>(1, root),
					source = std::vector<int>(num_vertices(graph), -1),
					visited = std::vector<char>(num_vertices(graph), 0)
					]() mutable {
						using result_type = std::optional<std::pair<int, int>>;
						if(dfs_stack.empty()) {
							return std::nullopt;
						}
						int curr = dfs_stack.back();
						dfs_stack.pop_back();

						for(const auto& v: adjacent_vertices(graph, curr)) {
							if(not visited[v]) {
								dfs_stack.push_back(v);
								source[v] = curr;
							}
						}
						visited[curr] = 1;
						return std::pair {source[curr], curr};
				}
			);
		}

	}

}


#endif //GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
