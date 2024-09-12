
#ifndef GRAPH2X_CORE_HPP
#define GRAPH2X_CORE_HPP

#include <utility>
#include <concepts>
#include <map>
#include <ranges>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <vector>

namespace g2x {

	using isize = std::ptrdiff_t;
	using usize = std::size_t;

	// this exists because std::vector<bool>
	using boolean = char;

	namespace detail {
		
		template<typename T>
		using cref_or_integral_value = std::conditional_t<std::is_integral_v<std::remove_cvref_t<T>>, T, const T&>;


		inline void hash_combine(std::size_t& seed) { }

		template <typename T, typename... Rest>
		inline void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
			hash_combine(seed, rest...);
		}

		template<typename... Ts>
		inline std::size_t combined_hash(const Ts&... vs) {
			std::size_t seed = 0;
			hash_combine(seed, vs...);
			return seed;
		}

		template<typename T>
		auto make_sorted_pair(const T& a, const T& b) -> std::pair<T, T> {
			if(a > b) {
				return {b, a};
			} else {
				return {a, b};
			}
		}
	}


	template<typename VtxIdT, typename EdgeIdT, bool IsDirected = true>
	struct edge_value {
		VtxIdT u;
		VtxIdT v;
		EdgeIdT i;

		using g2x_edge_value = void;
		using vertex_id_type = VtxIdT;
		using edge_id_type = EdgeIdT;

		template<int i>
		auto get(this auto&& self) {
			if constexpr (i == 0) {
				return self.u;
			} else if constexpr (i == 1) {
				return self.v;
			} else if constexpr (i == 2) {
				return self.i;
			}
		}

		[[nodiscard]] edge_value swap_to_first(const VtxIdT& vtx) const {
			if(u != vtx) {
				return {v, u, i};
			} else {
				return {u, v, i};
			}
		}

		[[nodiscard]] edge_value swap_to_second(const VtxIdT& vtx) const {
			if(v != vtx) {
				return {v, u, i};
			} else {
				return {u, v, i};
			}
		}

		[[nodiscard]] auto normalized_tuple() const {
			if constexpr (IsDirected) {
				return std::tuple {u, v, i};
			} else {
				if(u > v) return std::tuple{v, u, i};
				else return std::tuple{u, v, i};
			}
		}

		friend auto operator<=>(const edge_value& a, const edge_value& b) {
			return a.normalized_tuple() <=> b.normalized_tuple();
		}

		friend bool operator==(const edge_value& a, const edge_value& b) {
			return a.normalized_tuple() == b.normalized_tuple();
		}


	};

	template<typename VtxIdT, bool IsDirected = true>
	struct simplified_edge_value {
		VtxIdT u;
		VtxIdT v;

		using g2x_simplified_edge_value = void;
		using vertex_id_type = VtxIdT;
		using edge_id_type = std::pair<VtxIdT, VtxIdT>;

		template<int i>
		auto get(this auto&& self) {
			if constexpr (i == 0) {
				return self.u;
			} else if constexpr (i == 1) {
				return self.v;
			} else if constexpr (i == 2) {
				return self.normalized_pair();
			}
		}

		[[nodiscard]] simplified_edge_value swap_to_first(const VtxIdT& vtx) const {
			if(u != vtx) {
				return {v, u};
			} else {
				return {u, v};
			}
		}

		[[nodiscard]] simplified_edge_value swap_to_second(const VtxIdT& vtx) const {
			if(v != vtx) {
				return {v, u};
			} else {
				return {u, v};
			}
		}


		[[nodiscard]] auto normalized_pair() const {
			if constexpr (IsDirected) {
				return std::pair{u, v};
			} else {
				return detail::make_sorted_pair(u, v);
			}
		}

		[[nodiscard]] friend auto operator<=>(const simplified_edge_value& a, const simplified_edge_value& b) {
			return a.normalized_pair() <=> b.normalized_pair();
		}

		friend bool operator==(const simplified_edge_value& a, const simplified_edge_value& b) {
			return a.normalized_pair() == b.normalized_pair();
		}
	};
}

template<typename VtxIdT, typename EdgeIdT, bool IsDirected>
struct std::tuple_size<g2x::edge_value<VtxIdT, EdgeIdT, IsDirected>> {
	static constexpr size_t value = 3;
};

template<typename VtxIdT, bool IsDirected>
struct std::tuple_size<g2x::simplified_edge_value<VtxIdT, IsDirected>> {
	static constexpr size_t value = 3;
};

template<std::size_t N, typename VtxIdT, typename EdgeIdT, bool IsDirected>
struct std::tuple_element<N, g2x::edge_value<VtxIdT, EdgeIdT, IsDirected>> {
	using type = std::remove_cvref_t<decltype(std::declval<g2x::edge_value<VtxIdT, EdgeIdT>>().template get<N>())>;
};

template<std::size_t N, typename VtxIdT, bool IsDirected>
struct std::tuple_element<N, g2x::simplified_edge_value<VtxIdT, IsDirected>> {
	using type = std::remove_cvref_t<decltype(std::declval<g2x::simplified_edge_value<VtxIdT>>().template get<N>())>;
};


template<typename VtxIdT, typename EdgeIdT, bool IsDirected>
struct std::hash<g2x::edge_value<VtxIdT, EdgeIdT, IsDirected>> {
	static size_t operator()(const g2x::edge_value<VtxIdT, EdgeIdT, IsDirected>& e) {
		return std::apply([](auto&&... vs){return g2x::detail::combined_hash(vs...);}, e.normalized_tuple());
	}
};

template<typename VtxIdT, bool IsDirected>
struct std::hash<g2x::simplified_edge_value<VtxIdT, IsDirected>> {
	static size_t operator()(const g2x::simplified_edge_value<VtxIdT, IsDirected>& e) {
		auto p = e.normalized_pair();
		return g2x::detail::combined_hash(p.first, p.second);
	}
};



namespace g2x {

	namespace graph_traits {

		template<typename GraphT>
		struct is_directed {
			static constexpr bool value = requires {
				requires GraphT::is_directed;
			};
		};

		template<typename GraphT>
		inline constexpr bool is_directed_v = is_directed<GraphT>::value;



		template<typename GraphT>
		struct allows_loops {
			static constexpr bool value = requires {
				requires GraphT::allows_loops;
			};
		};

		template<typename GraphT>
		inline constexpr bool allows_loops_v = allows_loops<GraphT>::value;



		template<typename GraphT>
		struct allows_multiple_edges {
			static constexpr bool value = requires {
				requires GraphT::allows_multiple_edges;
			};
		};

		template<typename GraphT>
		inline constexpr bool allows_multiple_edges_v = allows_multiple_edges<GraphT>::value;



		template<typename GraphT>
		struct has_natural_vertex_numbering {
			static constexpr bool value = requires {
				requires GraphT::has_natural_vertex_numbering;
			};
		};

		template<typename GraphT>
		inline constexpr bool has_natural_vertex_numbering_v = has_natural_vertex_numbering<GraphT>::value;



		template<typename GraphT>
		struct has_natural_edge_numbering {
			static constexpr bool value = requires {
				requires GraphT::has_natural_edge_numbering;
			};
		};

		template<typename GraphT>
		inline constexpr bool has_natural_edge_numbering_v = has_natural_edge_numbering<GraphT>::value;



		template<typename GraphT>
		struct outgoing_edges_uv_sorted {
			static constexpr bool value = requires {
				requires GraphT::outgoing_edges_uv_sorted;
			};
		};

		template<typename GraphT>
		inline constexpr bool outgoing_edges_uv_sorted_v = outgoing_edges_uv_sorted<GraphT>::value;


		template<typename GraphT>
		struct outgoing_edges_pre_swapped {
			static constexpr bool value = requires {
				requires GraphT::outgoing_edges_pre_swapped;
			};
		};

		template<typename GraphT>
		inline constexpr bool outgoing_edges_pre_swapped_v = outgoing_edges_pre_swapped<GraphT>::value;


		template<typename GraphT>
		struct incoming_edges_pre_swapped {
			static constexpr bool value = requires {
				requires GraphT::incoming_edges_pre_swapped;
			};
		};

		template<typename GraphT>
		inline constexpr bool incoming_edges_pre_swapped_v = incoming_edges_pre_swapped<GraphT>::value;
	}

	auto unindexed(auto&& edge_range) {
		return edge_range | std::views::transform([&](auto&& edge) {
			auto&& [u, v, i] = edge;
			return std::pair {u, v};
		});;
	}


	template<typename GraphRefT>
	using edge_t = typename std::remove_cvref_t<GraphRefT>::edge_value_type;

	template<typename GraphRefT>
	using vertex_id_t = typename edge_t<GraphRefT>::vertex_id_type;

	template<typename GraphRefT>
	using edge_id_t = typename edge_t<GraphRefT>::edge_id_type;

	/*
	 * Returns the range consisting of all valid vertex IDs for a particular graph.
	 */
	template<typename GraphRefT>
	auto all_vertices(GraphRefT&& graph) {
		return graph.all_vertices();
	}

	/*
	 * Returns the range consisting of all edges in the graph, viewed as tuple-likes
	 * [src_vertex, dst_vertex, index].
	 */
	template<typename GraphRefT>
	auto all_edges(GraphRefT&& graph) {
		return graph.all_edges();
	}

	/*
	 * Returns the range consisting of all edges in the graph, viewed as tuple-likes
	 * [src_vertex, dst_vertex].
	 */
	template<typename GraphRefT>
	auto all_edges_unindexed(GraphRefT&& graph) {
		return unindexed(all_edges(graph));
	}

	/*
	 * Returns a range of all edges that are outgoing from v, viewed as tuple-likes
	 * [src_vertex, dst_vertex, index].
	 *
	 * src_vertex is GUARANTEED to be equivalent to v. That is, if the graph is undirected
	 * and outgoing_edges(4) is requested, a hypothetical edge (4, 1) will always be rendered
	 * as [4, 1, i] and not as [1, 4, i] regardless of how it is stored internally. The behavior
	 * of library functions is undefined if an implementation fails to meet this requirement.
	 */
	template<typename GraphRefT>
	auto outgoing_edges(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		using GraphT = std::remove_cvref_t<GraphRefT>;

		if constexpr (graph_traits::outgoing_edges_pre_swapped_v<GraphT>) {
			return graph.outgoing_edges(v);
		} else {
			return graph.outgoing_edges(v) | std::views::transform([&](const edge_t<GraphT>& edge) {
				return edge.swap_to_first(v);
			});
		}
	}

	/*
	 * Equivalent to unindexed(outgoing_edges(graph, v)).
	 */
	template<typename GraphRefT>
	auto outgoing_edges_unindexed(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return unindexed(outgoing_edges(graph, v));
	}

	/*
	 * Returns a range of all edges that are incoming to v. Similarly to the behavior of
	 * outgoing_edges, the second element of each tuple-like in the returned range is
	 * guaranteed to be equivalent to v.
	 *
	 * This is expected to be unimplemented in most cases. Only rely on this function's
	 * availability if fast access to the list of incoming edges is crucial.
	 */
	template<typename GraphRefT>
	auto incoming_edges(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		using GraphT = std::remove_cvref_t<GraphRefT>;

		if constexpr (graph_traits::incoming_edges_pre_swapped_v<GraphT>) {
			return graph.incoming_edges(v);
		} else {
			return graph.incoming_edges(v) | std::views::transform([&](const edge_t<GraphT>& edge) {
				return edge.swap_to_second(v);
			});
		}
		// return graph.incoming_edges(v);
	}


	/*
	 * Returns the value of the edge of a given index.
	 */
	template<typename GraphRefT>
	auto edge_at(GraphRefT&& graph, const edge_id_t<GraphRefT>& index) {
		using vid_t = vertex_id_t<GraphRefT>;
		using ev_t = edge_t<GraphRefT>;
		if constexpr(requires{typename ev_t::g2x_simplified_edge_value;}) {
			const auto& [u, v] = index;
			return ev_t{u, v};
		} else {
			return graph.edge_at(index);
		}
	}

	/*
	 * Returns the number of vertices of a graph.
	 */
	template<typename GraphRefT>
	auto num_vertices(GraphRefT&& graph) {
		if constexpr(requires{graph.num_vertices();}) {
			return graph.num_vertices();
		} else {
			return std::ranges::distance(all_vertices(graph));
		}
	}

	/*
	 * Returns the number of edges of a graph.
	 */
	template<typename GraphRefT>
	auto num_edges(GraphRefT&& graph) {
		if constexpr(requires{graph.num_edges();}) {
			return graph.num_edges();
		} else {
			return std::ranges::distance(all_edges(graph));
		}
	}

	/*
	 * Returns a range of all vertices that are adjacent to v.
	 */
	template<typename GraphRefT>
	auto adjacent_vertices(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		if constexpr (requires {graph.adjacent_vertices(v);}) {
			return graph.adjacent_vertices(v);
		} else {
			return outgoing_edges(graph, v) | std::views::transform([](auto&& e) {
				const auto& [u, v, i] = e;
				return v;
			});
		}
	}


	/*
	 * Returns whether a pair of vertices in a graph is adjacent.
	 */
	template<typename GraphRefT>
	bool is_adjacent(GraphRefT&& graph, const vertex_id_t<GraphRefT>& u, const vertex_id_t<GraphRefT>& v) {
		using GraphT = std::remove_cvref_t<GraphRefT>;
		if constexpr (requires{graph.is_adjacent(u, v);}) {
			return graph.is_adjacent(u, v);
		} else {
			auto&& adj_vertices = adjacent_vertices(graph, u);
			static constexpr bool adj_vertices_rar = std::ranges::random_access_range<decltype(adj_vertices)>;
			static constexpr bool is_sorted = graph_traits::outgoing_edges_uv_sorted_v<GraphT>;
			if constexpr (adj_vertices_rar && is_sorted) {
				return std::ranges::binary_search(adj_vertices, v);
			} else {
				return std::ranges::find(adj_vertices, v) != std::ranges::end(adj_vertices);
			}
		}
	}


	/*
	 * For undirected graphs, this yields the degree of a vertex.
	 * For directed graphs, this yields the outdegree of a vertex.
	 *
	 * If your code does not need to work with undirected graphs, use the
	 * 'outdegree' alias for clarity.
	 */
	template<typename GraphRefT>
	auto degree(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		using GraphT = std::remove_cvref_t<GraphRefT>;
		if constexpr (requires{graph.degree(v);}) {
			return graph.degree(v);
		} else if constexpr (requires{graph.outdegree(v);}) {
			return graph.outdegree(v);
		} else if constexpr (graph_traits::is_directed_v<GraphT>){
			return std::ranges::size(outgoing_edges(graph, v));
		} else {
			isize result = 0;
			for(const auto& [u1, v1]: outgoing_edges_unindexed(graph, v)) {
				if(u1 == v) ++result;
				if(v1 == v) ++result;
			}
			return result;
		}
	}

	/*
	 * Similar to "degree", but prioritizes the graph type's "outdegree" method
	 * if both "degree" and "outdegree" are present.
	 */
	template<typename GraphRefT>
	auto outdegree(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		if constexpr (requires{graph.outdegree(v);}) {
			return graph.outdegree(v);
		} else if constexpr (requires{graph.degree(v);}) {
			return graph.degree(v);
		} else {
			return std::ranges::size(outgoing_edges(graph));
		}
	}

	/*
	 * Yields the indegree of a vertex in a directed graph. Do not expect this to be available
	 * in the general case.
	 */
	template<typename GraphRefT>
	auto indegree(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		if constexpr (requires{graph.indegree(v);}) {
			return graph.indegree(v);
		} else {
			return std::ranges::size(incoming_edges(graph));
		}
	}




	/*
	 * Returns an object 'vl' where for a valid vertex 'v' of a graph 'g'
	 * the expression 'vl[v]' yields a reference to an object of type T
	 * owned by 'vl'. The initial value of such objects is unspecified.
	 */
	template<typename T, typename GraphRefT>
	auto create_vertex_labeling(GraphRefT&& graph) {
		if constexpr (requires{graph.template create_vertex_labeling<T>();}) {
			return graph.template create_vertex_labeling<T>();
		} else if constexpr (graph_traits::has_natural_vertex_numbering_v<std::remove_cvref_t<GraphRefT>>) {
			return std::vector<T>(num_vertices(graph));
		} else if constexpr (requires{std::unordered_map<vertex_id_t<GraphRefT>, T>{};}) {
			return std::unordered_map<vertex_id_t<GraphRefT>, T>{};
		} else {
			return std::map<vertex_id_t<GraphRefT>, T>{};
		}
	}

	/*
	 * Returns an object 'vl' where for a valid vertex 'v' of a graph 'g'
	 * the expression 'vl[v]' yields a reference to an object of type T
	 * owned by 'vl'. The initial value of all such objects is equivalent to "value".
	 */
	template<typename T, typename GraphRefT>
	auto create_vertex_labeling(GraphRefT&& graph, const T& value) {
		auto labeling = create_vertex_labeling<T>(graph);
		if constexpr (std::ranges::contiguous_range<decltype(labeling)>) {
			std::ranges::fill(labeling, value);
		} else {
			for(const auto& v: all_vertices(graph)) {
				labeling[v] = value;
			}
		}
		return labeling;
	}

	/*
	 * Returns an object 'el' where for a valid edge index 'e' of a graph 'g'
	 * the expression 'el[e]' yields a reference to an object of type T
	 * owned by 'el'. The initial value of such objects is not specified.
	 */
	template<typename T, typename GraphRefT>
	auto create_edge_labeling(GraphRefT&& graph) {
		if constexpr (requires{graph.template create_edge_labeling<T>();}) {
			return graph.template create_edge_labeling<T>();
		} else if constexpr (graph_traits::has_natural_edge_numbering_v<std::remove_cvref_t<GraphRefT>>) {
			return std::vector<T>(num_edges(graph));
		} else if constexpr (requires{std::unordered_map<edge_id_t<GraphRefT>, T>{};}) {
			return std::unordered_map<edge_id_t<GraphRefT>, T>{};
		} else {
			return std::map<edge_id_t<GraphRefT>, T>{};
		}
	}

	/*
	 * Returns an object 'el' where for a valid edge index 'e' of a graph 'g'
	 * the expression 'el[e]' yields a reference to an object of type T
	 * owned by 'el'. The initial value of such objects is equivalent to "value".
	 */
	template<typename T, typename GraphRefT>
	auto create_edge_labeling(GraphRefT&& graph, const T& value) {
		auto labeling = create_edge_labeling<T>(graph);
		if constexpr (std::ranges::contiguous_range<decltype(labeling)>) {
			std::ranges::fill(labeling, value);
		} else {
			for(const auto& [u, v, i]: all_edges(graph)) {
				labeling[i] = value;
			}
		}

		return labeling;
	}

	/*
	 * Creates a vertex in a graph and returns its index.
	 */
	template<typename GraphRefT>
	auto create_vertex(GraphRefT&& graph) -> vertex_id_t<GraphRefT> {
		return graph.create_vertex();
	}

	/*
	 * Creates a vertex in a graph with a given index, which is returned.
	 */
	template<typename GraphRefT>
	auto add_vertex(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) -> vertex_id_t<GraphRefT> {
		return graph.add_vertex(v);
	}

	/*
	 * Creates an edge between a pair of vertices. The index of the created edge is returned.
	 */
	template<typename GraphRefT>
	auto add_edge(GraphRefT&& graph, const vertex_id_t<GraphRefT>& u, const vertex_id_t<GraphRefT>& v) -> edge_id_t<GraphRefT> {
		return graph.add_edge(u, v);
	}

	/*
	 * Removes a specific vertex from a graph.
	 */
	template<typename GraphRefT>
	bool remove_vertex(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.remove_vertex(v);
	}

	/*
	 * Removes a specific edge from a graph.
	 */
	template<typename GraphRefT>
	bool remove_edge(GraphRefT&& graph, const edge_id_t<GraphRefT>& e) {
		return graph.remove_edge(e);
	}

	namespace detail {
		
		template<typename T, typename GraphRefT>
		concept range_of_vertices_for = requires(T vr) {
			requires std::ranges::range<T>;
			{*std::ranges::begin(vr)} -> std::convertible_to<vertex_id_t<GraphRefT>>;
		};
		
		template<typename T, typename GraphRefT>
		concept range_of_edges_for = requires(T vr) {
			requires std::ranges::range<T>;
			{*std::ranges::begin(vr)} -> std::convertible_to<edge_t<GraphRefT>>;
		};
		
		template<typename T, typename GraphRefT>
		concept range_of_edge_ids_for = requires(T vr) {
			requires std::ranges::range<T>;
			{*std::ranges::begin(vr)} -> std::convertible_to<edge_id_t<GraphRefT>>;
		};

		
	}

	template<typename T>
	concept edge = requires(T e) {
		requires std::tuple_size_v<T> == 3;
		{std::get<0>(e)} -> std::convertible_to<typename T::vertex_id_type>;
		{std::get<1>(e)} -> std::convertible_to<typename T::vertex_id_type>;
		{std::get<2>(e)} -> std::convertible_to<typename T::edge_id_type>;
	};


	template<typename T, typename GraphRefT>
	concept vertex_labeling = requires(T lc, vertex_id_t<GraphRefT> v) {
		{lc[v]};
	};

	template<typename T, typename GraphRefT>
	concept edge_labeling = requires(T lc, edge_id_t<GraphRefT> v) {
		{lc[v]};
	};


	template<typename T, typename GraphRefT, typename ValueT>
	concept vertex_labeling_of = requires(T lc, vertex_id_t<GraphRefT> v) {
		{lc[v]} -> std::convertible_to<ValueT>;
	};

	template<typename T, typename GraphRefT, typename ValueT>
	concept edge_labeling_of = requires(T lc, edge_id_t<GraphRefT> v) {
		{lc[v]} -> std::convertible_to<ValueT>;
	};
	
	template<typename T>
	concept graph = requires(T graph, vertex_id_t<T> v, edge_id_t<T> e) {
		{num_vertices(graph)} -> std::integral;
		{num_edges(graph)} -> std::integral;
		{is_adjacent(graph, v, v)} -> std::convertible_to<bool>;
		{adjacent_vertices(graph, v)} -> detail::range_of_vertices_for<T>;
		{outgoing_edges(graph, v)} -> detail::range_of_edges_for<T>;
		{all_vertices(graph)} -> detail::range_of_vertices_for<T>;
		{all_edges(graph)} -> detail::range_of_edges_for<T>;
		{edge_at(graph, e)} -> std::convertible_to<edge_t<T>>;
		{create_vertex_labeling<int>(graph, 0)} -> vertex_labeling<T>;
		{create_edge_labeling<int>(graph, 0)} -> edge_labeling<T>;
	};
	
	template<typename T>
	concept buildable_graph = graph<T> && requires(T graph, vertex_id_t<T> v, edge_id_t<T> e) {
		{add_vertex(graph, v)} -> std::convertible_to<vertex_id_t<T>>;
		{add_edge(graph, v, v)} -> std::convertible_to<edge_id_t<T>>;
	};

	template<typename T>
	concept reducible_graph = graph<T> && requires(T graph, vertex_id_t<T> v, edge_id_t<T> e) {
		{remove_vertex(graph, v)} -> std::convertible_to<bool>;
		{remove_edge(graph, e)} -> std::convertible_to<bool>;
	};

	template<typename T>
	concept mutable_graph = buildable_graph<T> && reducible_graph<T>;


	using vertex_count = std::optional<int>;
	inline constexpr vertex_count auto_num_vertices = std::nullopt;

}

#endif //GRAPH2X_CORE_HPP
