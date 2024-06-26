/* ggraph.cpp */

#include "ggraph.h"

void Ggraph::_register_methods() {
	register_method("add_node", &Ggraph::add_node);
	register_method("remove_node", &Ggraph::remove_node);
	register_method("add_edge", &Ggraph::add_edge);
	register_method("remove_edge", &Ggraph::remove_edge);

	register_method("adjacent", &Ggraph::adjacent);
	register_method("neighbours", &Ggraph::neighbours);
	register_method("nodes_edges", &Ggraph::nodes_edges);
	register_method("centroid", &Ggraph::centroid);
	register_method("subgraphs", &Ggraph::subgraphs);
	register_method("remove_subgraph", &Ggraph::remove_subgraph);
	register_method("setup_edge_matrix", &Ggraph::setup_edge_matrix);
	register_method("set_edge_matrix", &Ggraph::set_edge_matrix);
	register_method("get_edge_matrix", &Ggraph::get_edge_matrix);
	register_method("test_edge_matrix", &Ggraph::test_edge_matrix);

	register_property("_nodes", &Ggraph::_nodes, Dictionary());
	register_property("_edges", &Ggraph::_edges, Array());
}

void Ggraph::set_nodes(Dictionary nodes) {
	_nodes = nodes;
}

Dictionary Ggraph::get_nodes() {
	return _nodes;
}

void Ggraph::set_edges(Array edges) {
	_edges = edges;
}

Array Ggraph::get_edges() {
	return _edges;
}

Dictionary Ggraph::get_edge_matrix() {
	return edge_matrix->_get_data();
}

void Ggraph::set_edge_matrix(Ref<BitMap> p_edge_matrix) {
	edge_matrix = p_edge_matrix;
}

Ggraph::Ggraph() {
}

Ggraph::~Ggraph() {
}

void Ggraph::_init() {
	using_edge_matrix = false;
}

void Ggraph::setup_edge_matrix() {
	// This must be run after all the nodes are loaded.
	// The edge matrix will NOT update node additions or subtractions
	// Only edge additions
	edge_matrix = Ref<BitMap>(BitMap::_new());
	edge_matrix->create(Size2((real_t)_nodes.size(), (real_t)_nodes.size()));
	//Godot::print(String("EDGE MATRIX: ") + String::num_real(edge_matrix->get_size().x) + " " + String::num_real(edge_matrix->get_size().y));
	/*
	if (!edge_matrix.is_valid()) {
		if (edge_matrix->get_size().width != _nodes.size()) {
			edge_matrix->create(Size2((real_t)_nodes.size(), (real_t)_nodes.size()));
		}
	}*/
	
	if (edge_matrix.is_valid()) {
		// update edge_matrix
		for (int i = 0; i < _edges.size(); i++) {
			Dictionary edge = _edges[i];
			edge_matrix->set_bit(Point2(edge["a"], edge["b"]), true);
			edge_matrix->set_bit(Point2(edge["b"], edge["a"]), true);
		}
	}
	
	using_edge_matrix = true;
}

bool Ggraph::test_edge_matrix() {
	for (int i = 0; i < _edges.size(); i++) {
		Dictionary edge = _edges[i];
		assert(edge_matrix->get_bit(Vector2(edge["a"], edge["b"])));
		assert(edge_matrix->get_bit(Vector2(edge["b"], edge["a"])));
	}
	return true;
}

void Ggraph::add_node(int x, Vector3 position) {
	Dictionary node;
	node["position"] = position;
	node["force"] = Vector3();
	node["bond"] = 10.0;
	node["group"] = 0;
	node["anchor"] = false;

	_nodes[x] = node;
}

void Ggraph::remove_node(int x) {
	for(int i = _edges.size()-1; i != -1; i--) {
		Dictionary edge = _edges[i];
		if ((int)edge["a"] == x || (int)edge["b"] == x) {
			_edges.remove(i);
		}
	}
	_nodes.erase(x);

	// offset every key higher

	if (using_edge_matrix) {
		for (int i = 0; i < edge_matrix->get_size().height; i++) {
			edge_matrix->set_bit(Vector2(x, i), false);
			edge_matrix->set_bit(Vector2(i, x), false);
		}
	}
}

void Ggraph::add_edge(int x, int y) {
	Dictionary edge;
	edge["a"] = x;
	edge["b"] = y;

	_edges.append(edge);

	if (using_edge_matrix) {
		edge_matrix->set_bit(Vector2(x, y), true);
		edge_matrix->set_bit(Vector2(y, x), true);
	}
}

void Ggraph::remove_edge(int x, int y) {
	for(int i = _edges.size()-1; i != -1; i--) {
		//printf("%d --- %d == %d --- %d\n", (int)edge["a"], (int)edge["b"], x, y);
		Dictionary edge = _edges[i];
		if (((int)edge["a"] == x && (int)edge["b"] == y) || ((int)edge["a"] == y && (int)edge["b"] == x)) {
			_edges.remove(i);
			//return; // There should not be copies of the same edge
		}
	}

	if (using_edge_matrix) {
		edge_matrix->set_bit(Vector2(x, y), false);
		edge_matrix->set_bit(Vector2(y, x), false);
	}
}

bool Ggraph::adjacent(int x, int y) {
	if (using_edge_matrix) {
		return edge_matrix->get_bit(Vector2(x,y)) || edge_matrix->get_bit(Vector2(y,x));
	}

	for(int i = 0; i < _edges.size(); i++) {
		Dictionary edge = _edges[i];
		if (((int)edge["a"] == x && (int)edge["b"] == y) || ((int)edge["a"] == y && (int)edge["b"] == x)) {
			return true;
		}
	}
	return false;
}

PoolIntArray Ggraph::neighbours(int x) {
	PoolIntArray n;

	if (using_edge_matrix) {
		for (int i = 0; i < edge_matrix->get_size().height; i++) {
			if (edge_matrix->get_bit(Vector2(x, i)) || edge_matrix->get_bit(Vector2(i, x))) {
				n.append(i);
			}
		}
	} else {
		for(int i = 0; i < _edges.size(); i++) {
			Dictionary edge = _edges[i];
			if (x == (int)edge["a"])
				n.append((int)edge["b"]);
			if (x == (int)edge["b"])
				n.append((int)edge["a"]);
		}
	}
	return n;
}

Vector3 Ggraph::centroid() {
	if (_nodes.empty()) {
		return Vector3::ZERO;
	}
	
	Vector3 sum = Vector3();
	Array values = _nodes.values();
	for (int i = 0; i < values.size(); i++) {
		sum += static_cast<Dictionary>(values[i])["position"];
	}
	return sum / (float)values.size();
}

Array Ggraph::subgraphs() {
	if (_nodes.empty()) {
		return Array();
	}
	Array r_subgraphs = Array();
	Array nodes = _nodes.keys().duplicate(true);

	// Single node, no subgraphs
	if (nodes.size() == 1) {
		Array g;
		g.append(nodes[0]);
		r_subgraphs.append(g);
		return r_subgraphs;
	}

	Array queue;
	queue.append(nodes[0]);
	Array visited;
	visited.append(nodes[0]);
	nodes.erase(nodes[0]);

	//int64_t t = 0;
	//int64_t t2 = 0;
	while (nodes.size() > 0) { // quick
		while (!queue.empty()) { // quick
			int node = queue.pop_back(); // front = DFS

			//uint64_t begin = OS::get_singleton()->get_ticks_usec();
			if (node == -1) {
				Godot::print("WHY IS THERE A -1 HERE???");
			}
			PoolIntArray node_neighbours = neighbours(node); //neighbours(node); // slow but always performed once per node
			
			//uint64_t end = OS::get_singleton()->get_ticks_usec();
			//t = t + (end - begin);

			for (int i = 0; i < node_neighbours.size(); i++) {
				int neighbour = node_neighbours[i];
				if (!visited.has(neighbour)) { // if neighbour not visited
					visited.append(neighbour); // mark as visited
					queue.append(neighbour); // plan for exploration
					nodes.erase(neighbour); // remove it from the nodes
				}
			}
		}

		r_subgraphs.append(visited.duplicate(true));

		if (nodes.size() == 1) {
			Array g;
			g.append(nodes[0]);
			r_subgraphs.append(g);
			nodes.erase(nodes[0]);
			break;
		} else if (nodes.size() > 1) {
			visited.clear();
			visited.append(nodes.pop_back());
			queue.clear();
			queue.append(visited[0]);
		}
	}
	
	//Godot::print(String("GET NEIGHBOURS milliseconds: ") + String::num_real((double)t / 1000.0f));
	return r_subgraphs;
}

Array Ggraph::nodes_edges(Array nodes) {
	Array edges = Array();
	for(int i = 0; i < _edges.size(); i++) {
		Dictionary edge = _edges[i];
		if (nodes.has((int)edge["a"]) && nodes.has((int)edge["b"])) {
			edges.append(_edges[i]);
		}
	}
	return edges;
}

Ggraph* Ggraph::remove_subgraph(Array nodes) {
	// remove and return subgraph
	// remaps shape owners incrementally
	Ggraph *subgraph = Ggraph::_new();
	Dictionary mapping = Dictionary();
	int idx = 0;
	for (int n = 0; n < nodes.size(); n++) {
		int node = nodes[n];
		mapping[node] = idx;
		subgraph->_nodes[idx] = static_cast<Dictionary>(_nodes[node]);
		
		Dictionary d = subgraph->_nodes[idx];
		d["force"] = Vector3();
		
		_nodes.erase(node);
		idx++;
	}
	
	for (int i = _edges.size()-1; i >= 0; i--) {

		Dictionary edge = _edges[i];
		if (nodes.has((int)edge["a"]) && nodes.has((int)edge["b"])) {
			Dictionary new_edge;
			new_edge["a"] = mapping[(int)edge["a"]];
			new_edge["b"] = mapping[(int)edge["b"]];
			subgraph->_edges.append(new_edge);
			_edges.remove(i);
		}
	}
	
	return subgraph;
}