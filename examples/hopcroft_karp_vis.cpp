
#include <../include/graph2x.hpp>
#include <iostream>

auto& g_random = g2x::algo::config::hopcroft_karp.random_generator;

void render_bfsnet_to_tikz(std::ostream& os, auto&& graph, auto&& partitions, auto&& matching, auto&& bfs_levels, auto&& aug_set) {

	std::unordered_map<int, float> lvl_ypos;
	struct vec3f {
		float x=0.f, y=0.f;
	};
	auto vtx_positions = g2x::create_vertex_property(graph, vec3f{});
	auto out_it = std::ostream_iterator<char>{os};

	std::format_to(out_it, "\\tikz {{\n");


	for(const auto& v: g2x::all_vertices(graph)) {

		if(bfs_levels[v] < 0) {
			continue;
		}

		std::format_to(out_it,
			R"|(	\node ({}) [circle, draw, scale=0.6] at ({:.2f}, {:.2f}) {{{}}};)|",
			v,
			float(bfs_levels[v])*2,
			lvl_ypos[bfs_levels[v]] += 1.0f,
			std::format("d={}", bfs_levels[v]));
		std::format_to(out_it, "\n");
	}


	std::format_to(out_it, "\t\\graph[nodes={{circle, draw}}] {{\n");

	for(const auto& [u, v, i]: g2x::all_edges(graph)) {
		// std::println("{}/{},{}/{},{} ||||||||||||||||||||| \n", u, bfs_levels[u], v, bfs_levels[v], i);
		int u1 = u;
		int v1 = v;
		if(bfs_levels[u1] < 0 || bfs_levels[v1] < 0) {
			continue;
		}
		if(bfs_levels[u1] > bfs_levels[v1]) {
			std::swap(u1, v1);
		}

		bool active = true;

		if(bfs_levels[v1] - bfs_levels[u1] != 1) {
			continue;
		}
		if(bfs_levels[u1] % 2 != matching[i]) {
			active = false;
		}

		std::vector<std::string> styles;
		if(matching[i]) {
			styles.push_back("ultra thick");
		}
		if(aug_set[i]) {
			styles.push_back("blue");
		}
		if(not active) {
			styles.push_back("lightgray");
			styles.push_back("dashed");
		}
		std::string style_str = styles | std::views::join_with(',') | std::ranges::to<std::string>();

		std::format_to(out_it,
			R"|(		({}) ->[{}] ({});)|",
				u1, style_str, v1);
		std::format_to(out_it, "\n");



	}

	std::format_to(out_it, "\t}};\n");

	std::format_to(out_it, "}}");

}

void manual_hopcroft_karp(auto&& graph) {
	auto partitions = g2x::algo::bipartite_decompose(graph).value();
	auto matching = g2x::create_edge_property(graph, char(false));

	while(true) {
		auto bfs_levels = g2x::algo::detail::hopcroft_karp_bfs_stage(graph, partitions, matching, nullptr);
		auto aug_set = g2x::algo::find_bipartite_augmenting_set(graph, partitions, matching);
		auto aug_set_map = g2x::create_edge_property(graph, char(false));
		for(const auto& i: aug_set) {
			aug_set_map[i] = true;
		}
		render_bfsnet_to_tikz(std::cout, graph, partitions, matching, bfs_levels, aug_set_map);
		std::cout << "\n\n\n\n-------------------------------------------------\n\n\n\n";

		if(aug_set.empty()) {
			break;
		}

		for(const auto& idx: aug_set) {
			matching[idx] = !matching[idx];
		}

	}

}

int main() {

	g2x::algo::config::hopcroft_karp.random_generator.engine = []() {
		thread_local std::mt19937_64 gen(1007);
		return gen();
	};

	auto edges = g2x::graph_gen::average_degree_bipartite_generator(15, 15, 3.0, g_random);
	auto graph = g2x::create_graph<g2x::basic_graph>(edges);

	manual_hopcroft_karp(graph);



}