# GraphGizmo.gd
extends EditorSpatialGizmo


# You can store data in the gizmo itself (more useful when working with handles).
var gizmo_size = 2.0

var index_node_mapping = []

func redraw():
	clear()
	
	var fracture = get_spatial_node()
	if fracture._graph == null:
		return
	
	var nodes: Dictionary = fracture._graph._nodes
	var edges: Array = fracture._graph._edges
	
	if not nodes.empty():
		# Lines
		if not edges.empty():
			var lines = PoolVector3Array()
			
			for edge in edges:
				if edge != null:
					lines.push_back(nodes[edge.a].position)
					lines.push_back(nodes[edge.b].position)
			
			var material = get_plugin().get_material("main", self)
			
			if not lines.empty():
				add_lines(lines, material, false)
		
		# Handles
		var handles = PoolVector3Array()
		var anchor_handles = PoolVector3Array()
		
		var handles_material = get_plugin().get_material("handles", self)
		var anchor_material = get_plugin().get_material("anchor", self)
		
		for n in range(nodes.keys().size()):
			var key = nodes.keys()[n]
			var handle = PoolVector3Array()
			handle.push_back(nodes[key].position)
			add_handles(handle, anchor_material if nodes[key].anchor else handles_material)


# You should implement the rest of handle-related callbacks

func get_handle_name(index):
	return 'Glue Graph Node'

func get_handle_value(index):
	return index

func commit_handle(index: int, restore, cancel: bool=false):
	var node = get_spatial_node()
	if node._graph != null:
		node._graph._nodes[index].anchor = !node._graph._nodes[index].anchor
		#print("HANDLE UPDATE GRAPH NODE AT INDEX: %d [ANCHOR = %s]" % [index, node._graph._nodes[index].anchor])

func is_handle_highlighted(index):
	pass

func set_handle(index, camera, point):
	pass
