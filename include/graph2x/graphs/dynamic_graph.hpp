
#ifndef GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA
#define GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA

#include "../core.hpp"

#include <unordered_map>
#include <unordered_set>

namespace g2x {

	/*
	 * A fully mutable graph.
	 *
	 * Creation:
	 * Adjacency check:
	 * Pass over outgoing_edges:
	 * Pass over adjacent_vertices:
	 * Pass over all_vertices:
	 * Pass over all_edges:
	 * Edge index lookup:
	 */

	template<typename VIdxT, typename EIdxT, bool IsDirected = false>
	class general_dynamic_graph {
	private:

		std::unordered_map<VIdxT, std::unordered_set<VIdxT>> nodes;

	public:



	};

}

#endif //GRAPH2X_DYNAMIC_GRAPH_HPP_523F9F607EB1463A8BBA601621395DFA
