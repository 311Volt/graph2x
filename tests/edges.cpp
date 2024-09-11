
#include "tests_pch.hpp"

namespace {

	TEST(edge_tests, undir_full_edge_compare) {
        g2x::edge_value<int, int, false> e1 {2,3,0}, e2 {3,2,0}, e3 {2,3,5}, e4 {2,4,5}, e5{5,1,6};
		EXPECT_EQ(e1, e2);
		EXPECT_NE(e1, e3);
		EXPECT_LT(e1, e4);
		EXPECT_GT(e4, e1);
		EXPECT_LT(e5, e1);
    }

	TEST(edge_tests, undir_full_edge_hash) {
		g2x::edge_value<int, int, false> e1 {2,3,0}, e2 {3,2,0}, e3{2,3,1};
		std::hash<decltype(e1)> hasher{};
		EXPECT_EQ(hasher(e1), hasher(e2));
		// EXPECT_NE(hasher(e1), hasher(e3));
	}

	TEST(edge_tests, dir_full_edge_compare) {
		g2x::edge_value<int, int, true> e1 {2,3,0}, e2 {3,2,0}, e3 {2,3,5}, e4 {2,4,5}, e5{5,1,6};
		EXPECT_NE(e1, e2);
		EXPECT_NE(e1, e3);
		EXPECT_LT(e1, e4);
		EXPECT_GT(e4, e1);
		EXPECT_GT(e5, e1);
	}

	TEST(edge_tests, undir_simple_edge_compare) {
		g2x::simplified_edge_value<int, false> e1 {0,0}, e2{1,0}, e3{0,1}, e4{1,1}, e5{2,0};
		EXPECT_EQ(e2, e3);
		EXPECT_LT(e1, e2);
		EXPECT_GT(e4, e1);
		EXPECT_LT(e5, e4);
	}

	TEST(edge_tests, undir_simple_edge_hash) {
		g2x::simplified_edge_value<int, false> e2{1,0}, e3{0,1};
		std::hash<decltype(e2)> hasher;
		EXPECT_EQ(e2, e3);
	}

	TEST(edge_tests, dir_simple_edge_compare) {
		g2x::simplified_edge_value<int, true> e1 {0,0}, e2{1,0}, e3{0,1}, e4{1,1}, e5{2,0};
		EXPECT_NE(e2, e3);
		EXPECT_LT(e1, e2);
		EXPECT_GT(e4, e1);
		EXPECT_GT(e5, e4);
	}

}