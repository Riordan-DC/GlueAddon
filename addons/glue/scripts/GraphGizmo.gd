# GraphGizmo.gd
extends EditorNode3DGizmo


# You can store data in the gizmo itself (more useful when working with handles).
var gizmo_size = 2.0

var index_node_mapping = []

func _redraw():
	clear()
	
	var fracture = get_node_3d()
	if fracture._graph == null:
		return
	
	var nodes: Dictionary = fracture._graph._nodes
	var edges: Array = fracture._graph._edges
	
	if not nodes.is_empty():
		# Lines
		if not edges.is_empty():
			var lines = PackedVector3Array()
			
			for edge in edges:
				if edge != null:
					lines.push_back(nodes[edge.a].position)
					lines.push_back(nodes[edge.b].position)
			
			var material = get_plugin().get_material("main", self)
			
			if not lines.is_empty():
				add_lines(lines, material, false)
		
		# Handles
		var handles = PackedVector3Array()
		var anchor_handles = PackedVector3Array()
		
		var handles_material = get_plugin().get_material("handles", self)
		var anchor_material = get_plugin().get_material("anchor", self)
		
		for n in range(nodes.keys().size()):
			var key = nodes.keys()[n]
			var handle = PackedVector3Array()
			var ids = PackedInt32Array()
			handle.push_back(nodes[key].position)
			add_handles(handle, anchor_material if nodes[key].anchor else handles_material, ids)


# You should implement the rest of handle-related callbacks

func _get_handle_name(_index, _secondary):
	return 'Glue Graph Node'

func _get_handle_value(index, _secondary):
	return index

#func _commit_handle(index: int, _secondary, restore, cancel: bool=false):
#	var node = get_node_3d()
#	if node._graph != null:
#		node._graph._nodes[index].anchor = !node._graph._nodes[index].anchor
		#print("HANDLE UPDATE GRAPH NODE AT INDEX: %d [ANCHOR = %s]" % [index, node._graph._nodes[index].anchor])

func _begin_handle_action(index, _secondary):
	var node = get_node_3d()
	if node._graph != null:
		node._graph._nodes[index].anchor = !node._graph._nodes[index].anchor
		#print("HANDLE UPDATE GRAPH NODE AT INDEX: %d [ANCHOR = %s]" % [index, node._graph._nodes[index].anchor])

func _is_handle_highlighted(_index, _secondary):
	pass

func _set_handle(_index, _secondary, _camera, _point):
	pass
