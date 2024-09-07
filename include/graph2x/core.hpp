
#ifndef GRAPH2X_CORE_HPP
#define GRAPH2X_CORE_HPP

#include <utility>
#include <concepts>
#include <map>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <vector>

namespace g2x {
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
	}


	template<typename VtxIdT, typename EdgeIdT>
	struct edge_value {
		VtxIdT u;
		VtxIdT v;
		EdgeIdT i;

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

		friend auto operator<=>(const edge_value& a, const edge_value& b) {
			return std::tuple {a.u, a.v, a.i} <=> std::tuple {b.u, b.v, b.i};
		}


	};

	template<typename VtxIdT>
	struct simplified_edge_value {
		VtxIdT u;
		VtxIdT v;

		using vertex_id_type = VtxIdT;
		using edge_id_type = std::pair<VtxIdT, VtxIdT>;

		template<int i>
		auto get(this auto&& self) {
			if constexpr (i == 0) {
				return self.u;
			} else if constexpr (i == 1) {
				return self.v;
			} else if constexpr (i == 2) {
				return std::pair {self.u, self.v};
			}
		}

		[[nodiscard]] friend auto operator<=>(const simplified_edge_value& a, const simplified_edge_value& b) {
			return std::tuple{a.u, a.v} <=> std::tuple{b.u, b.v};
		}
	};

	// template<int N, typename VtxIdT, typename EdgeIdT>
	// auto&& get(const edge_value<VtxIdT, EdgeIdT>& e) {
	// 	return e.template get<N>();
	// }
	// template<int N, typename VtxIdT>
	// auto&& get(const simplified_edge_value<VtxIdT>& e) {
	// 	return e.template get<N>();
	// }
}

template<typename VtxIdT, typename EdgeIdT>
struct std::tuple_size<g2x::edge_value<VtxIdT, EdgeIdT>> {
	static constexpr size_t value = 3;
};

template<typename VtxIdT>
struct std::tuple_size<g2x::simplified_edge_value<VtxIdT>> {
	static constexpr size_t value = 3;
};

template<std::size_t N, typename VtxIdT, typename EdgeIdT>
struct std::tuple_element<N, g2x::edge_value<VtxIdT, EdgeIdT>> {
	using type = std::remove_cvref_t<decltype(std::declval<g2x::edge_value<VtxIdT, EdgeIdT>>().template get<N>())>;
};

template<std::size_t N, typename VtxIdT>
struct std::tuple_element<N, g2x::simplified_edge_value<VtxIdT>> {
	using type = std::remove_cvref_t<decltype(std::declval<g2x::simplified_edge_value<VtxIdT>>().template get<N>())>;
};


template<typename VtxIdT, typename EdgeIdT>
struct std::hash<g2x::edge_value<VtxIdT, EdgeIdT>> {
	static size_t operator()(const g2x::edge_value<VtxIdT, EdgeIdT>& e) {
		return g2x::detail::combined_hash(e.u, e.v, e.i);
	}
};

template<typename VtxIdT>
struct std::hash<g2x::simplified_edge_value<VtxIdT>> {
	static size_t operator()(const g2x::simplified_edge_value<VtxIdT>& e) {
		return g2x::detail::combined_hash(e.u, e.v);
	}
};



namespace g2x {

	namespace graph_traits {
		template<typename GraphT>
		inline constexpr bool is_directed = false;

		template<typename GraphT>
			requires (requires{GraphT::is_directed;})
		inline constexpr bool is_directed<GraphT> = GraphT::is_directed;

		template<typename GraphT>
		inline constexpr bool natural_vertex_numbering = false;

		template<typename GraphT>
			requires (requires{GraphT::natural_vertex_numbering;})
		inline constexpr bool natural_vertex_numbering<GraphT> = GraphT::natural_vertex_numbering;

		template<typename GraphT>
		inline constexpr bool natural_edge_numbering = false;

		template<typename GraphT>
			requires (requires{GraphT::natural_edge_numbering;})
		inline constexpr bool natural_edge_numbering<GraphT> = GraphT::natural_edge_numbering;
	}


	template<typename GraphRefT>
	using edge_t = typename std::remove_cvref_t<GraphRefT>::edge_value_type;

	template<typename GraphRefT>
	using vertex_id_t = typename edge_t<GraphRefT>::vertex_id_type;

	template<typename GraphRefT>
	using edge_id_t = typename edge_t<GraphRefT>::edge_id_type;

	/*
	 * Returns the range consisting of all vertices in the graph.
	 */
	template<typename GraphRefT>
	auto all_vertices(GraphRefT&& graph) {
		return graph.all_vertices();
	}

	/*
	 * Returns the range consisting of all edges in the graph, viewed as tuples
	 * [src_vertex, dst_vertex, index].
	 */
	template<typename GraphRefT>
	auto all_edges(GraphRefT&& graph) {
		return graph.all_edges();
	}

	/*
	 * Returns a range of all edges that are outgoing from v.
	 * The first element of each of the returned tuples is guaranteed to
	 * be equivalent to v.
	 */
	template<typename GraphRefT>
	auto outgoing_edges(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.outgoing_edges(v);
	}

	/*
	 * Returns a range of all edges that are incoming to v.
	 * This is expected to be unimplemented in most cases, but some digraph types
	 * may provide an implementation.
	 */
	template<typename GraphRefT>
	auto incoming_edges(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.incoming_edges(v);
	}


	/*
	 * Returns the value of the edge of a given index.
	 */
	template<typename GraphRefT>
	auto edge_at(GraphRefT&& graph, const edge_id_t<GraphRefT>& index) {
		return graph.edge_at(index);
	}

	/*
	 * Returns the number of vertices of a graph.
	 */
	template<typename GraphRefT>
	auto num_vertices(GraphRefT&& graph) {
		if constexpr(requires{graph.num_vertices();}) {
			return graph.num_vertices();
		} else {
			return std::ranges::size(all_vertices(graph));
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
			return std::ranges::size(all_edges(graph));
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
		if constexpr (requires{graph.is_adjacent(u, v);}) {
			return graph.is_adjacent(u, v);
		} else {
			auto&& adj_vertices = adjacent_vertices(graph);
			static constexpr bool adj_vertices_rar = std::ranges::random_access_range<decltype(adj_vertices)>;
			if constexpr (adj_vertices_rar && requires{graph.adj_vertex_ordering();}) {
				return std::ranges::binary_search(adj_vertices, v, graph.adj_vertex_ordering());
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
		if constexpr (requires{graph.degree(v);}) {
			return graph.degree(v);
		} else if constexpr (requires{graph.outdegree(v);}) {
			return graph.outdegree(v);
		} else {
			return std::ranges::size(outgoing_edges(graph));
		}
	}

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
		} else if constexpr (graph_traits::natural_vertex_numbering<std::remove_cvref_t<GraphRefT>>) {
			return std::vector<T>(num_vertices(graph));
		} else if constexpr (requires{std::unordered_map<vertex_id_t<GraphRefT>, T>{};}) {
			return std::unordered_map<vertex_id_t<GraphRefT>, T>{};
		} else {
			return std::map<vertex_id_t<GraphRefT>, T>{};
		}
	}

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
	 * owned by 'el'.
	 */
	template<typename T, typename GraphRefT>
	auto create_edge_labeling(GraphRefT&& graph) {
		if constexpr (requires{graph.template create_edge_labeling<T>();}) {
			return graph.template create_edge_labeling<T>();
		} else if constexpr (graph_traits::natural_edge_numbering<std::remove_cvref_t<GraphRefT>>) {
			return std::vector<T>(num_edges(graph));
		} else if constexpr (requires{std::unordered_map<edge_id_t<GraphRefT>, T>{};}) {
			return std::unordered_map<edge_id_t<GraphRefT>, T>{};
		} else {
			return std::map<edge_id_t<GraphRefT>, T>{};
		}
	}

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
		{std::get<0>(e)} -> std::same_as<typename T::vertex_id_type>;
		{std::get<1>(e)} -> std::same_as<typename T::vertex_id_type>;
		{std::get<2>(e)} -> std::same_as<typename T::edge_id_type>;
	};


	template<typename T, typename GraphRefT>
	concept vertex_labeling_for = requires(T lc, vertex_id_t<GraphRefT> v) {
		{lc[v]};
	};

	template<typename T, typename GraphRefT>
	concept edge_labeling_for = requires(T lc, edge_id_t<GraphRefT> v) {
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
		{create_vertex_labeling<int>(graph, 0)} -> vertex_labeling_for<T>;
		{create_edge_labeling<int>(graph, 0)} -> edge_labeling_for<T>;
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

	// this exists because std::vector<bool>
	using boolean = char;

	using isize = std::ptrdiff_t;
	using usize = std::size_t;

	using vertex_count = std::optional<int>;
	inline constexpr vertex_count auto_num_vertices = std::nullopt;

}

#endif //GRAPH2X_CORE_HPP
