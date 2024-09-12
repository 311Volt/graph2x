
#ifndef GRAPH2X_DYNAMIC_LIST_GRAPH_HPP
#define GRAPH2X_DYNAMIC_LIST_GRAPH_HPP

#include "../core.hpp"
#include <list>

namespace g2x {

	template<std::integral VIdxT, std::integral EIdxT = int, bool IsDirected = false>
	class general_dynamic_list_graph {
	public:
		using edge_value_type = edge_value<VIdxT, EIdxT, IsDirected>;
		using vertex_id_type = typename edge_value_type::vertex_id_type;
		using edge_id_type = typename edge_value_type::edge_id_type;



	private:
		std::list<edge_value_type> edge_list_;
		using list_iterator = typename decltype(edge_list_)::iterator;
		std::vector<list_iterator> edge_index_;
		std::vector<boolean> edge_active_;
		std::vector<boolean> vertex_active_;
		std::vector<std::list<edge_id_type>> adj_lists;

		isize num_vertices_ = 0;
		isize num_edges_ = 0;

	};

}


#endif //GRAPH2X_DYNAMIC_LIST_GRAPH_HPP
