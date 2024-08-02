# graph2x

A general-purpose C++23 graph library. Its aim is to leverage modern C++ features to provide an easy-to-use library that covers common use-cases.

This library is **work-in-progress**. Most graph types and algorithms that were planned for this library are currently missing.

## Usage examples

### Print all the neighbors of a vertex
```c++
void print_neighbors_of_vertex(g2x::graph auto&& graph, int u)
{
	std::println("The neighbors of {} are:", u);
	for(const auto& v: g2x::adjacent_vertices(graph, u)) {
		std::print("{} ", v);
	}
	std::println("");
}
```


### Create a graph from a range of edges
```c++
std::vector<std::pair<int, int>> edges = {
	{0, 2},		{0, 4},		{0, 5},
	{1, 4},		{1, 5},
	{2, 3},		{2, 4},
	{4, 5}
}

g2x::static_simple_graph my_graph{6, edges};
std::println("Read a graph with {} vertices and {} edges",
	g2x::num_vertices(graph),
	g2x::num_edges(graph)
);
```
