
#ifndef GRAPH2X_ALGO_MISC_HPP
#define GRAPH2X_ALGO_MISC_HPP

#include "../core.hpp"
#include "search.hpp"

namespace g2x {

	namespace algo {

		auto count_connected_components(graph auto&& graph) {
			isize result = 0;
			depth_first_search dfs(graph);
			for(const auto& v: all_vertices(graph)) {
				++result;
				while(dfs.next_vertex());
			}
			return result;
		}

	}

}

#endif //GRAPH2X_ALGO_MISC_HPP
