#include "tests_common.hpp"

namespace {

	template<typename T>
	struct average {
		T total = 0;
		int samples = 0;

		void add(const T& value) {
			total += value;
			++samples;
		}

		[[nodiscard]] T get() const {
			return total / samples;
		}
	};

	TEST(graph_generators, iota_random_subset_density) {
		std::mt19937_64 rng(311);
		int n = 1500;
		auto n1 = std::ranges::distance(g2x::detail::iota_random_subset(1500, 0.1, rng));
		EXPECT_LT(std::abs(n1 - 150), 20);
	}

	TEST(graph_generators, avg_deg_should_fall_within_tolerance) {
		std::mt19937_64 rng(311);
		average<double> avg_deg;
		for(int i=0; i<10; i++) {
			auto edges = g2x::graph_gen::random_edges_deg(1000, 3.0, false, rng);
			g2x::nested_vec_graph graph(1000, edges);
			for(const auto& v: g2x::all_vertices(graph)) {
				avg_deg.add(g2x::degree(graph, v));
			}
		}
		EXPECT_LT(std::abs(avg_deg.get() - 3.0), 0.02);
	}

}
