extends Resource
class_name GlueGraph

#class GlueNode:
#	var position: Vector3
#	var value: float
#
#class GlueEdge:
#	var a: int
#	var b: int
#	var value: float


export var _nodes: Dictionary = {}
export var _edges: Array = []

enum GlueNodeKeys {
	POSITION = 0, # Vector3
	FORCE = 1, # Vector3
	BOND = 2, # float
	ANCHOR = 3, # bool
	GROUP = 4 # int
}

var max_bond = 10.0

func _ready():
	pass

func add_node(shape_id: int, position: Vector3):
	var node = {
		"position": position,
		"force": Vector3.ZERO,
		"bond": 10.0,
		"group": 0,
		"anchor": false
	}
	_nodes[shape_id] = node
	
func add_edge(a: int, b: int):
	var edge = {
		"a": a,
		"b": b
	}
	_edges.append(edge)

func remove_node(shape_id: int) -> void:
	for i in range(_edges.size()-1, -1, -1):
		if _edges[i].a == shape_id or _edges[i].b == shape_id:
			_edges.remove(i)
	_nodes.erase(shape_id)

func remove_edge(a: int, b: int) -> void:
	for i in range(_edges.size()-1, -1, -1):
		var edge = _edges[i]
		if (edge.a == a and edge.b == b) or (edge.a == b and edge.b == a):
			_edges.remove(i)

func adjacent(a: int, b: int) -> bool:
	for edge in _edges:
		if (edge.a == a and edge.b == b) or (edge.a == b and edge.b == a):
			return true
	return false

func centroid() -> Vector3:
	var sum = Vector3.ZERO
	var nodes = _nodes.values()
	for node in nodes:
		sum += node.position
	return sum / nodes.size()

func neighbours(node: int):
	var neighbours = []
	for edge in _edges:
		if node == edge.a:
			neighbours.append(edge.b)
		if node == edge.b:
			neighbours.append(edge.a)
	return neighbours

func subgraphs() -> Array:
	var subgraphs = []
	var nodes = _nodes.keys().duplicate(true)
	var queue = [nodes[0]]
	var visited = [queue[0]]
	nodes.erase(queue[0])
	
	while nodes.size() > 0:
		while not queue.empty():
			var node = queue.pop_front()
			for neighbour in neighbours(node):
				if not visited.has(neighbour):
					visited.append(neighbour)
					queue.append(neighbour)
					nodes.erase(neighbour)
		subgraphs.append(visited)
		visited = [nodes.pop_front()]
		queue = [visited[0]]
	
	if visited[0] != null:
		subgraphs.append([visited[0]])
	
	return subgraphs

func nodes_edges(nodes: Array) -> Array:
	var edges = []
	for edge in _edges:
		if nodes.has(edge.a) and nodes.has(edge.b):
			edges.append(edge)
	return edges

func remove_subgraph(nodes: Array):
	# remove and return subgraph
	# remaps shape owners incrementally
	var subgraph = get_script().new()
	var node_data = {}
	var mapping = {}
	var idx = 0
	for n in range(nodes.size()):
		var node = nodes[n]
		mapping[node] = idx
		node_data[idx] = _nodes[node]
		
		node_data[idx].force = Vector3.ZERO
		
		_nodes.erase(node)
		idx += 1
	
	var edges = []
	for i in range(_edges.size()-1, -1, -1):
		if nodes.has(_edges[i].a) and nodes.has(_edges[i].b):
			_edges[i].a = mapping[_edges[i].a]
			_edges[i].b = mapping[_edges[i].b]
			edges.append(_edges[i])
			_edges.remove(i)
	
	subgraph._nodes = node_data
	subgraph._edges = edges
	return subgraph
