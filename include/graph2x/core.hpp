
#ifndef GRAPH2X_CORE_HPP
#define GRAPH2X_CORE_HPP

#include <utility>
#include <concepts>
#include <ranges>

namespace g2x {
	
	namespace detail {
		
		template<typename T>
		using cref_or_integral_value = std::conditional_t<std::is_integral_v<std::remove_cvref_t<T>>, T, const T&>;
		
	}
	
	template<typename GraphRefT>
	using vertex_id_t = typename std::remove_cvref_t<GraphRefT>::vertex_id_type;
	
	template<typename GraphRefT>
	using edge_id_t = typename std::remove_cvref_t<GraphRefT>::edge_id_type;

	template<typename VtxIdT, typename EdgeIdT>
	struct edge_value {
		VtxIdT u;
		VtxIdT v;
		EdgeIdT i;
	};

	template<typename VtxIdT, typename EdgeIdT>
	struct edge_view {
		detail::cref_or_integral_value<VtxIdT> u;
		detail::cref_or_integral_value<VtxIdT> v;
		detail::cref_or_integral_value<EdgeIdT> i;
	};

	template<typename GraphRefT>
	using edge_t = edge_view<vertex_id_t<GraphRefT>, edge_id_t<GraphRefT>>;
	
	
	template<typename GraphRefT>
	auto num_vertices(GraphRefT&& graph) {
		return graph.num_vertices();
	}

	template<typename GraphRefT>
	auto num_edges(GraphRefT&& graph) {
		return graph.num_edges();
	}

	template<typename GraphRefT>
	bool is_adjacent(GraphRefT&& graph, const vertex_id_t<GraphRefT>& u, const vertex_id_t<GraphRefT>& v) {
		return graph.is_adjacent(u, v);
	}

	template<typename GraphRefT>
	auto adjacent_vertices(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.adjacent_vertices(v);
	}

	template<typename GraphRefT>
	auto outgoing_edges(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.outgoing_edges(v);
	}

	template<typename GraphRefT>
	auto all_vertices(GraphRefT&& graph) {
		return graph.all_vertices();
	}

	template<typename GraphRefT>
	auto all_edges(GraphRefT&& graph) {
		return graph.all_edges();
	}

	template<typename GraphRefT>
	auto edge_at(GraphRefT&& graph, const edge_id_t<GraphRefT>& index) {
		return graph.edge_at(index);
	}

	template<typename T, typename GraphRefT>
	auto create_vertex_labeling(GraphRefT&& graph, T value = {}) {
		return graph.template create_vertex_labeling<T>(value);
	}

	template<typename T, typename GraphRefT>
	auto create_edge_labeling(GraphRefT&& graph, T value = {}) {
		return graph.template create_edge_labeling<T>(value);
	}
	
	template<typename GraphRefT>
	auto create_vertex(GraphRefT&& graph) -> vertex_id_t<GraphRefT> {
		return graph.create_vertex();
	}
	
	template<typename GraphRefT>
	auto add_vertex(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) -> vertex_id_t<GraphRefT> {
		return graph.add_vertex(v);
	}
	
	template<typename GraphRefT>
	auto add_edge(GraphRefT&& graph, const vertex_id_t<GraphRefT>& u, const vertex_id_t<GraphRefT>& v) -> edge_id_t<GraphRefT> {
		return graph.add_edge(u, v);
	}
	
	template<typename GraphRefT>
	bool remove_vertex(GraphRefT&& graph, const vertex_id_t<GraphRefT>& v) {
		return graph.remove_vertex(v);
	}
	
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
		{create_vertex_labeling(graph, 0)} -> vertex_labeling_for<T>;
		{create_edge_labeling(graph, 0)} -> edge_labeling_for<T>;
	};
	
	template<typename T>
	concept buildable_graph = graph<T> && requires(T graph, vertex_id_t<T> v, edge_id_t<T> e) {
		{add_vertex(graph, v)} -> std::convertible_to<vertex_id_t<T>>;
		{add_edge(graph, v, v)} -> std::convertible_to<edge_id_t<T>>;
	};

	template<typename T>
	concept mutable_graph = buildable_graph<T> && requires(T graph, vertex_id_t<T> v, edge_id_t<T> e) {
		{remove_vertex(graph, v)} -> std::convertible_to<bool>;
		{remove_edge(graph, e)} -> std::convertible_to<bool>;
	};

	// this exists because std::vector<bool>
	using boolean = char;

}

#endif //GRAPH2X_CORE_HPP
