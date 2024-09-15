
#ifndef GRAPH2X_MATCHINGS_3REGULAR_HPP
#define GRAPH2X_MATCHINGS_3REGULAR_HPP

#include "search.hpp"
#include "../core.hpp"
#include "graph2x/graphs/dynamic_list_graph.hpp"
#include "graph2x/graphs/nested_vec_graph.hpp"

namespace g2x {
	namespace algo {

		auto compute_articulation_points_brute_force(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;
			std::vector<vid_t> articulation_points;

			std::optional<vid_t> current_candidate;
			breadth_first_search bfs(graph, [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				if(current_candidate) {
					return u != *current_candidate && v != *current_candidate;
				} else {
					return true;
				}
			});

			isize num_connected_components = 0;
			for(const auto& v: all_vertices(graph)) {
				if(bfs.get_vertex_state(v) == vertex_search_state::unvisited) {
					++num_connected_components;
					bfs.add_vertex(v);
					while(bfs.next_vertex());
				}
			}
			bfs.reset();

			for(const auto& candidate: all_vertices(graph)) {
				current_candidate = candidate;
				bfs.reset();

				std::vector<vid_t> component_roots;
				for(const auto& v: all_vertices(graph)) {
					if(v == candidate) {
						continue;
					}
					if(bfs.get_vertex_state(v) == vertex_search_state::unvisited) {
						component_roots.push_back(v);
						bfs.add_vertex(v);
						while(bfs.next_vertex());
					}
				}
				if(component_roots.size() > num_connected_components) {
					articulation_points.emplace_back(candidate);
				}
			}

			return articulation_points;
		}

		auto compute_articulation_points(graph auto&& graph) {

			using vid_t = vertex_id_t<decltype(graph)>;

			auto depths = create_vertex_labeling<int>(graph, -1);
			auto lowpoints = create_vertex_labeling<int>(graph, -1);
			auto visited = create_vertex_labeling<boolean>(graph, false);
			std::vector<vid_t> articulation_points;

			for(const auto& root: all_vertices(graph)) {
				if(visited[root]) {
					continue;
				}

				[&](this auto const& self, const vid_t* parent, const vid_t& v, int depth) -> void {

					visited[v] = true;
					depths[v] = depth;
					lowpoints[v] = depth;
					std::optional<vid_t> v1, v2;

					if(parent) {
						v2 = std::exchange(v1, *parent);
					}

					for(const auto& child: adjacent_vertices(graph, v)) {
						if(not visited[child]) {
							self(&v, child, depth+1);
							if(not parent || lowpoints[child] >= depth) {
								v2 = std::exchange(v1, child);
							}
							lowpoints[v] = std::min(lowpoints[v], lowpoints[child]);
						} else if(parent && *parent != child) {
							lowpoints[v] = std::min(lowpoints[v], depths[child]);
						}
					}
					if(v1 && v2) {
						articulation_points.push_back(v);
					}

				}(nullptr, root, 0);
			}

			return articulation_points;
		}

		/*
		 * Returns a tuple (n, T, E), where:
		 *
		 * n is the number of articulation points in the input graph
		 *
		 * T is the block-cut tree of an undirected graph 'graph', where:
		 *    - vertices [0; n) represent articulation points
		 *    - vertices [n; |V(T)|) represent blocks
		 *
		 * E is an edge labeling of T, where for each edge-id i in T
		 * E[i] is an edge-id into 'graph' that corresponds to any one edge
		 * that connects the corresponding block and articulation point.
		 */
		auto create_block_cut_graph(graph auto&& graph)
			requires (not graph_traits::is_directed_v<std::remove_cvref_t<decltype(graph)>>)
		{

			using vid_t = vertex_id_t<decltype(graph)>;
			using eid_t = edge_id_t<decltype(graph)>;

			auto articulation_points = compute_articulation_points(graph);

			auto is_articulation_point = create_vertex_labeling<boolean>(graph, false);
			auto vertex_colors = create_vertex_labeling<vid_t>(graph);
			auto edge_color_opts = create_edge_labeling<std::optional<vid_t>>(graph, std::nullopt);

			depth_first_search dfs(graph, [&](auto&& edge) {
				const auto& [u, v, i] = edge;
				return not is_articulation_point[v]; //can only walk away from articulation points
			});

			auto block_cut_graph = create_graph<general_nested_vec_graph<vid_t, eid_t>>();
			auto equivalent_graph_edge = std::vector<eid_t>{};
			auto expand_and_insert = []<typename T>(std::vector<T>& vec, isize idx, const T& value) {
				if(vec.size() < idx+1) {
					vec.resize(idx+1);
				}
				vec[idx] = value;
			};

			for(const auto& ap: articulation_points) {
				is_articulation_point[ap] = true;
				vertex_colors[ap] = create_vertex(block_cut_graph);
				dfs.add_vertex(ap);
			}


			while(const auto& e_opt = dfs.next_edge()) {
				const auto& [u, v, i] = *e_opt;
				if(is_articulation_point[u]) { //new biconnected component
					vertex_colors[v] = create_vertex(block_cut_graph);
				} else { //dfs order - must have come from an already-colored vertex
					vertex_colors[v] = vertex_colors[u];
				}
			}

			for(const auto& ap: articulation_points) {
				for(const auto& [u, v, i]: outgoing_edges(graph, ap)) {

					if(is_articulation_point[v]) { //edge u-v represents a biconnected component of size 0
						if(u >= v) {
							continue;
						}
						auto blk = create_vertex(block_cut_graph);
						auto e1 = create_edge(block_cut_graph, vertex_colors[u], blk);
						auto e2 = create_edge(block_cut_graph, vertex_colors[v], blk);
						expand_and_insert(equivalent_graph_edge, e1, i);
						expand_and_insert(equivalent_graph_edge, e2, i);
					} else {
						auto eid = create_edge(block_cut_graph, vertex_colors[u], vertex_colors[v]);
						expand_and_insert(equivalent_graph_edge, eid, i);
					}
				}
			}

			using bct_t = decltype(block_cut_graph);
			using ege_t = decltype(equivalent_graph_edge);

			struct result_t {
				isize num_articulation_points;
				bct_t block_cut_graph;
				ege_t equivalent_graph_edge;
			};

			return result_t {
				.num_articulation_points = isize(articulation_points.size()),
				.block_cut_graph = block_cut_graph,
				.equivalent_graph_edge = equivalent_graph_edge
			};

		}


		namespace detail {
			auto common_and_aux(const auto& u1, const auto& v1, const auto& u2, const auto& v2) {
				if(u1 == u2) {
					return std::tuple {u1, v1, v2};
				}
				if(u1 == v2) {
					return std::tuple {u1, v1, u2};
				}
				if(v1 == u2) {
					return std::tuple {v1, u1, v2};
				}
				return std::tuple {v1, u1, u2};
			}
		}

		/* Makes a graph biconnected by inserting 4-cycles in-between its biconnected components. */
		void transform_into_biconnected(graph auto& graph) requires requires {
			requires not graph_traits::is_directed_v<std::remove_cvref_t<decltype(graph)>>;
			requires graph_traits::supports_edge_deletion_v<std::remove_cvref_t<decltype(graph)>>;
			requires graph_traits::supports_edge_creation_v<std::remove_cvref_t<decltype(graph)>>;
			requires graph_traits::supports_vertex_creation_v<std::remove_cvref_t<decltype(graph)>>;
		}
		{

			auto bcg = create_block_cut_graph(graph);
			depth_first_search dfs(bcg.block_cut_graph);

			auto reduced_to_first = create_vertex_labeling<boolean>(bcg.block_cut_graph, false);

			for(const auto& root: all_vertices(bcg.block_cut_graph)) {
				if(dfs.get_vertex_state(root) == vertex_search_state::visited) {
					continue;
				}
				dfs.add_vertex(root);

				bool first_block = true;
				while(auto v_opt = dfs.next_vertex()) {
					if(*v_opt >= bcg.num_articulation_points) {
						continue; //block vertex - skip
					}


					auto out_edges_view =
						std::views::repeat(outgoing_edges(bcg.block_cut_graph, *v_opt), 2)
						| std::views::join
						| std::views::adjacent<2>;

					for(const auto& [eb1, eb2]: out_edges_view) {
						const auto& [cut, blk_a, ib1] = eb1;
						const auto& [_cut, blk_b, ib2] = eb2;

						if(first_block) {
							reduced_to_first[blk_a] = true;
							first_block = false;
						}
						if(reduced_to_first[blk_a] == reduced_to_first[blk_b]) {
							continue;
						}

						auto [ug1, vg1, ig1] = edge_at(graph, bcg.equivalent_graph_edge[ib1]);
						auto [ug2, vg2, ig2] = edge_at(graph, bcg.equivalent_graph_edge[ib2]);
						auto [a, v1, v2] = detail::common_and_aux(ug1, vg1, ug2, vg2);

						auto b = create_vertex(graph);
						auto c = create_vertex(graph);
						auto d = create_vertex(graph);
						auto e = create_vertex(graph);

						create_edge(graph, v1, b);
						create_edge(graph, b, c);
						create_edge(graph, c, d);
						create_edge(graph, d, e);
						create_edge(graph, e, b);
						create_edge(graph, d, v2);

						reduced_to_first[blk_a] = true;
						reduced_to_first[blk_b] = true;
					}
				}
			}
		}


		enum class transfer_matching_mode_t {
			all,
			none,
			any,
			ignore
		};

		template<typename EIdxT>
		struct matching_reduction_node {
			std::array<EIdxT, 2> reduced_edges;
			EIdxT original_edge;
		};


		void transform_into_subcubic(graph auto& graph, auto&& on_reduction) requires requires {
			requires graph_traits::supports_edge_deletion_v<std::remove_cvref_t<decltype(graph)>>;
			requires graph_traits::supports_edge_creation_v<std::remove_cvref_t<decltype(graph)>>;
			requires graph_traits::supports_vertex_creation_v<std::remove_cvref_t<decltype(graph)>>;
			requires std::invocable<decltype(on_reduction), matching_reduction_node<edge_id_t<decltype(graph)>>>;

		}
		{
			using vid_t = vertex_id_t<decltype(graph)>;
			using eid_t = edge_id_t<decltype(graph)>;


			for(const auto& v: all_vertices(graph)) {
				std::vector<eid_t> edges_to_collapse =
					indices(outgoing_edges(graph, v))
					| std::ranges::to<std::vector>();

				while(edges_to_collapse.size() >= 4) {
					auto [ug1, vg1, eav1] = edge_at(graph, edges_to_collapse.back()); edges_to_collapse.pop_back();
					auto [ug2, vg2, eav2] = edge_at(graph, edges_to_collapse.back()); edges_to_collapse.pop_back();

					auto [a, v1, v2] = detail::common_and_aux(ug1, vg1, ug2, vg2);

					auto b = create_vertex(graph);
					auto c = create_vertex(graph);

					auto eab = create_edge(graph, a, b);
					auto ebc = create_edge(graph, b, c);
					auto ecv1 = create_edge(graph, c, v1);
					auto ecv2 = create_edge(graph, c, v2);

					if(not remove_edge(graph, eav1)) {
						throw std::runtime_error("this should never happen");
					}
					if(not remove_edge(graph, eav2)) {
						throw std::runtime_error("this should never happen");
					}

					on_reduction(matching_reduction_node<eid_t> {
						.reduced_edges = {ecv1, eab},
						.original_edge = eav1
					});
					on_reduction(matching_reduction_node<eid_t> {
						.reduced_edges = {ecv2, eab},
						.original_edge = eav2
					});

					edges_to_collapse.push_back(eab);
				}
			}
		}

		auto reduce_bipartite_to_biconnected_subcubic(graph auto const& graph)
			requires graph_traits::has_natural_edge_numbering_v<std::remove_cvref_t<decltype(graph)>>
		{
			using vid_t = vertex_id_t<decltype(graph)>;
			using eid_t = edge_id_t<decltype(graph)>;

			std::vector<matching_reduction_node<eid_t>> reduction_steps;

			auto ext_graph = create_graph<general_dynamic_list_graph<vid_t, eid_t>>(graph);
			transform_into_biconnected(ext_graph);
			transform_into_subcubic(ext_graph, [&](const matching_reduction_node<eid_t>& node) {
				reduction_steps.push_back(node);
			});

			return std::pair{std::move(ext_graph), reduction_steps};
		}

		template<typename EIdxT>
		void transfer_matching(
			graph auto&& original_graph,
			auto& original_matching,
			const auto& reduced_matching,
			const std::vector<matching_reduction_node<EIdxT>>& reduction_steps)
		{
			auto mut_reduced_matching = reduced_matching;

			for(const matching_reduction_node<EIdxT>& step: std::views::reverse(reduction_steps)) {
				//std::println("{} -> {} {}", step.reduced_edge, step.original_edge, bool(mut_reduced_matching[step.reduced_edge]));
				mut_reduced_matching[step.original_edge] =
					   mut_reduced_matching[step.reduced_edges[0]]
					&& mut_reduced_matching[step.reduced_edges[1]];
			}

			for(const auto& [u, v, i]: all_edges(original_graph)) {
				original_matching[i] = mut_reduced_matching[i];
			}
		}

	}
}

#endif //GRAPH2X_MATCHINGS_3REGULAR_HPP
