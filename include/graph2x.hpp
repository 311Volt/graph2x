
#ifndef GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
#define GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85

#include <concepts>
#include <vector>
#include <span>
#include <ranges>
#include <unordered_set>

namespace g2x {

	/*
	 * graphs:
	 * adjacency lists
	 * edge lists
	 * adjacency matrix
	 *
	 * graph free function interface:
	 * 	num_vertices(graph) -> int
	 * 	num_edges(graph) -> int
	 * 	is_adjacent(graph, v1, v2) -> bool
	 * 	adjacent_vertices(graph, v) -> range<int>
	 * 	outgoing_edges(graph, v) -> range<EdgeType>
	 * 	all_vertices(graph) -> range<int>
	 * 	all_edges(graph) -> range<EdgeType>
	 *
	 * graph mutation:
	 *  add_vertex(graph) -> VIdxT
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

	template<typename VIdxT>
	struct undirected_edge {
		VIdxT v1;
		VIdxT v2;
		
		bool operator==(const undirected_edge& other) const {
			return (v1 == other.v1 && v2 == other.v2) || (v1 == other.v2 && v2 == other.v1);
		}
	};

	template<typename VIdxT>
	struct directed_edge {
		VIdxT v_src;
		VIdxT v_dst;
		
		bool operator==(const directed_edge& other) const {
			return (v_src == other.v_src && v_dst == other.v_dst);
		}
	};
	

	template<std::integral VIdxT = int, typename EdgeT = undirected_edge<VIdxT>>
	class static_graph {
		std::vector<VIdxT> storage;
		std::vector<std::span<VIdxT>> segments;
	public:
	};

	template<std::integral VIdxT = int, typename EdgeT = undirected_edge<VIdxT>>
	class adjmatrix_graph {
		VIdxT size;
		std::vector<char> adj_matrix;
	public:
	};



}


#endif //GRAPH2X_GRAPH2X_HPP_F28D1E1186DE47298F5FDE26C930CB85
