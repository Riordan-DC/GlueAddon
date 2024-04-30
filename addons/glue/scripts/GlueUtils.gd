extends Node

static func glue(obj, scene_root, save_path: String="res://addons/glue/graphs/", ground_parent:Node=null):
	obj._graph = Ggraph.new()
	obj._graph._nodes = {}
	obj._graph._edges = []
	var world = obj.get_world_3d()
	var space_state = world.direct_space_state
	
	# Ground bodies
	var ground_bodies = []
	if ground_parent:
		ground_bodies = ground_parent.get_tree().get_nodes_in_group("ground")
		for body_idx in range(ground_bodies.size()):
			var body = ground_bodies[body_idx]
			if not body is StaticBody3D:
				ground_bodies.remove(body_idx)
			else:
				ground_bodies[body_idx] = body.get_instance_id()
#		for child in ground_parent.get_children():
#			if "ground" in child.name.to_lower():
#				if child is StaticBody:
#					ground_bodies.append(child.get_instance_id())
#				else:
#					for sub_child in child.get_children():
#						if sub_child is StaticBody:
#							ground_bodies.append(sub_child.get_instance_id())
		print(ground_bodies)
	
	var shape_owners: Array = obj.get_shape_owners()
	if shape_owners.is_empty():
		# Build collision shapes from children meshes
		for child in obj.get_children():
			if child is MeshInstance3D:
				var convexshape = child.mesh.create_convex_shape(true, true)
				var shape = CollisionShape3D.new()
				shape.shape = convexshape
				
				obj.add_child(shape)
				shape.set_owner(scene_root)
				shape.transform.origin = child.transform.origin
				
				obj.remove_child(child)
				shape.add_child(child)
				child.set_owner(scene_root)
				child.transform = Transform3D.IDENTITY
	
	for owner_id in obj.get_shape_owners():
		var shape_obj: CollisionShape3D = obj.shape_owner_get_owner(owner_id)
		var shape_id = owner_id
		var shape = obj.shape_owner_get_shape(owner_id, 0)
		var query = PhysicsShapeQueryParameters3D.new()
		query.set_shape(shape)
		query.collision_mask = 0xFFFFFFFFF
		query.transform = shape_obj.global_transform
		query.margin = obj.glue_margin# + 0.2
		query.exclude = [shape_obj]
		var adj_shapes = space_state.intersect_shape(query, 32)
		
		var debug_mesh = shape_obj.shape.get_debug_mesh()
#		print(shape_obj, " -> ", debug_mesh.get_surface_count())
		var shape_pos = mesh_centroid(debug_mesh) + shape_obj.transform.origin
		obj._graph.add_node(shape_id, shape_pos)
		
		for adj_shape in adj_shapes:
			if adj_shape['collider'].get_instance_id() != obj.get_instance_id():
				if ground_parent:
					if adj_shape['collider'].get_instance_id() in ground_bodies:
#						print("ground found")
						obj._graph._nodes[shape_id].anchor = true
				continue
			
			var adj_shape_id = adj_shape["shape"]
			if adj_shape_id != shape_id:
				if not obj._graph.adjacent(shape_id, adj_shape_id):
					obj._graph.add_edge(shape_id, adj_shape_id)
	
	var graph_filename = save_path + "%s_graph.tres" % [obj.name]
	print(graph_filename)
	ResourceSaver.save(obj._graph, graph_filename)
	obj.update_gizmos()

static func mesh_centroid(mesh: Mesh) -> Vector3:
	var verts: Array = mesh.surface_get_arrays(ArrayMesh.ARRAY_VERTEX)
	var sum: Vector3 = Vector3.ZERO
	var vert_count = 0
	for v in range(verts.size()):
		if verts[v] != null:
			for vert in verts[v]:
				sum += vert
				vert_count += 1
	return sum / float(vert_count)
