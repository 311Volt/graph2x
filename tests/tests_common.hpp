
#ifndef GRAPH2X_TESTS_PCH_HPP
#define GRAPH2X_TESTS_PCH_HPP

#include <gtest/gtest.h>
#include <graph2x.hpp>

template class g2x::general_basic_graph<int, int, false>;
template class g2x::general_basic_graph<int, int, true>;
template class g2x::general_dense_graph<int, false>;
template class g2x::general_dense_graph<int, true>;
template class g2x::general_nested_vec_graph<int, int, false>;
template class g2x::general_nested_vec_graph<int, int, true>;
template class g2x::general_dynamic_graph<int, int, false>;
template class g2x::general_dynamic_graph<int, int, true>;

#define GRAPH2X_EXPAND(A) A
#define GRAPH2X_CONCAT_NX(A, B) A ## B
#define GRAPH2X_CONCAT(A, B) GRAPH2X_CONCAT_NX(A, B)

#endif //GRAPH2X_TESTS_PCH_HPP
