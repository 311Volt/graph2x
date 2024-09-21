# graph2x

A general-purpose C++23 graph library that aims to let the programmer think about graphs in a pure, abstract way while 
still allowing for performant code.

This library is originally made for my master's thesis and is missing most basic algorithms that are outside of its
scope. However, I believe it is already very usable and provides a solid core interface that makes it easy to write
fully generic graph algorithms.

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
	{0, 2}, {0, 4},	{0, 5},
	{1, 4},	{1, 5},
	{2, 3},	{2, 4},
	{4, 5}
}

auto my_graph = g2x::create_graph<g2x::basic_graph>(edges);
std::println("Read a graph with {} vertices and {} edges",
	g2x::num_vertices(graph),
	g2x::num_edges(graph)
);
```

### Breadth-first search
```c++
g2x::breadth_first_search bfs(graph);
while(auto v_opt = bfs.next_vertex()) {
    std::println("Visiting {}", *v_opt);
}
```

## Graph interface

TODO

## Vertex and edge properties

TODO