
#include <graph2x.hpp>
#include <graph2x_lab/graph2x_lab.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>


struct bfs_performance_test {

	int num_vertices;

	struct x_axis {
		int num_edges;
	};

	struct y_axis {
		double time_us_g2x;
		double time_us_boost;
		// double time_us_stdgraph;
		// double time_us_koala;
	};

	y_axis eval(const x_axis& x, auto&& random) {

		auto edges = g2x::graph_gen::edge_cardinality_generator(num_vertices, x.num_edges, false, random);
		auto g2x_graph = g2x::create_graph<g2x::basic_graph>(edges);

		boost::adjacency_list<> boost_graph {size_t(num_vertices)};
		for(const auto& [u, v]: edges) {
			add_edge(int(u), int(v), boost_graph);
		}
		y_axis result;

		g2x::lab::stopwatch sw;
		volatile int i = 0;
		for(const auto& v: g2x::algo::simple_vertices_bfs(g2x_graph, 0)) {
			++i;
		}
		result.time_us_g2x = sw.peek() * 1000000.0;



		sw = {};
		class bfs_visitor_t: public boost::default_bfs_visitor {
		public:
			void discover_vertex(int s, const decltype(boost_graph) &g) {

			}
		};
		std::vector<boost::default_color_type> color_map(num_vertices);
		breadth_first_search(boost_graph, 0, boost::visitor(bfs_visitor_t{}));
		result.time_us_boost = sw.peek() * 1000000.0;

		return result;
	}

};


int main() {

	g2x::lab::execute_test<bfs_performance_test>({
		.short_title = "bfs-perf-g2x-boost",
		.title = "Czas przeszukania wszerz dla $|V| = 1000$",
		.samples_per_point = 200,
		.test_instance = {
			.num_vertices = 1000
		},
		.x_axis = g2x::lab::linspace(500, 100000, 100),
		.save_to_pgfplots = true,
	});

}