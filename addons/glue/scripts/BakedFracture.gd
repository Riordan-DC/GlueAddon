tool
extends RigidBody


# Scene Tree Structure
# BakedFracture
# - Shape 1
# 	- Mesh 1
# - Shape 2
# 	- Mesh 2

export(bool) var glue_debug_graph = false setget glue_set_debug_graph
var _debug_drawer = null
var _debug_line_mat = SpatialMaterial.new()
var _debug_point_mat = SpatialMaterial.new()

var bullet_backend = ProjectSettings.get("physics/3d/physics_engine") in ["DEFAULT", "Bullet"]
var GGraph = preload("res://addons/glue/gdnative/ggraph_class.gdns")
var center_changed = false

# UNUSED
export(bool) var flex = false
export(float) var flex_strength = 0.5

# Glue margin
export(float) var glue_margin = 0.005
# Strength of each bond
export(float) var glue_strength = 1000.0
# Glue anchor nodes stop its owner from moving
export(bool) var anchor_enabled = false setget set_anchor_enabled
# Anchors will not recieve force
export(bool) var anchor_static = false
# start locked to ground
export(Resource) var _graph = null


func glue_set_debug_graph(val):
	glue_debug_graph = val
	if _debug_drawer != null:
		_debug_drawer.visible = glue_debug_graph

func _ready():
	#graph_test(GGraph)
	self.angular_damp = 0.0000001
	self.contact_monitor = true
	if self.contacts_reported == 0:
		self.contacts_reported = 16
	self.custom_integrator = !bullet_backend
	if not Engine.editor_hint:
		#Engine.time_scale = 0.05
		if  _graph != null:
			if glue_debug_graph:
				glue_debug_setup()
				glue_debug_draw()
			
			_graph = _graph.duplicate(true)
			_graph.setup_edge_matrix()
			
			#_graph.max_bond = glue_strength
			for k in _graph._nodes.keys():
				_graph._nodes[k].bond = glue_strength
		
		recalculate_center_of_mass()

func glue():
	_graph = GGraph.new()
	_graph._nodes = {}
	_graph._edges = []
	var world = get_world()
	var space_state = world.direct_space_state
	
	var shape_owners: Array = get_shape_owners()
	if shape_owners.empty():
		# Build collision shapes from children meshes
		for child in get_children():
			if child is MeshInstance:
				var convexshape = child.mesh.create_convex_shape(true, true)
				var shape = CollisionShape.new()
				shape.shape = convexshape
				
				add_child(shape)
				shape.owner = get_tree().edited_scene_root
				shape.transform.origin = child.transform.origin
				
				remove_child(child)
				shape.add_child(child)
				child.owner = get_tree().edited_scene_root
				child.transform = Transform.IDENTITY
	
	for owner_id in get_shape_owners():
		var shape_obj: CollisionShape = shape_owner_get_owner(owner_id)
		var shape_id = owner_id
		var shape = shape_owner_get_shape(owner_id, 0)
		var query = PhysicsShapeQueryParameters.new()
		query.set_shape(shape)
		query.transform = shape_obj.global_transform
		query.margin = glue_margin
		query.exclude = [shape_obj]
		var adj_shapes = space_state.intersect_shape(query, 32)
		
		var shape_pos = mesh_centroid(shape_obj.shape.get_debug_mesh()) + shape_obj.transform.origin
		_graph.add_node(shape_id, shape_pos)
		
		for adj_shape in adj_shapes:
			if adj_shape['collider'].get_instance_id() != get_instance_id():
				continue
			var adj_shape_id = adj_shape["shape"]
			if adj_shape_id != shape_id:
				if not _graph.adjacent(shape_id, adj_shape_id):
					_graph.add_edge(shape_id, adj_shape_id)
	
	var graph_filename = self.get_script().get_path().get_base_dir() + "/graphs/%s_graph.tres" % [name]
	print(graph_filename)
	ResourceSaver.save(graph_filename, _graph)
	update_gizmo()

func set_anchor_enabled(val):
	anchor_enabled = val
	axis_lock_angular_x = val
	axis_lock_angular_y = val
	axis_lock_angular_z = val
	axis_lock_linear_x = val
	axis_lock_linear_y = val
	axis_lock_linear_z = val

func _process(delta):
	if glue_debug_graph:
		if _debug_drawer != null:
			if Engine.get_idle_frames() % 20 == 0:
				glue_debug_draw()
		else:
			glue_debug_setup()
	
	if bullet_backend:
		if center_changed and not Engine.is_in_physics_frame():
			recalculate_center_of_mass()
			center_changed = false
	
func _integrate_forces(state: PhysicsDirectBodyState):
	if not state.sleeping:
		for contact in range(state.get_contact_count()):
			var shape_id = state.get_contact_local_shape(contact)
			shape_id = shape_find_owner(shape_id)
			#var shape_obj = shape_owner_get_owner(shape_id)
			
			if anchor_static:
				if _graph._nodes[shape_id].anchor:
					continue
			
			var contact_normal = state.get_contact_local_normal(contact)
			var contact_body = state.get_contact_collider_object(contact)

			var speed_difference := linear_velocity
			#var mass = 1.0
			
			
			if contact_body is RigidBody:
				speed_difference -= contact_body.linear_velocity
				#mass = contact_body.mass
			
			var force_vector = speed_difference * contact_normal
			var force_power = force_vector.length_squared()
			
			# an anchor is true while it contacts its anchor object
			
			if force_power > 10.0:
				_graph._nodes[shape_id].force += force_vector
			
			#var volume = lerp(-20, 0, min(force_power, 1000) / 1000)
		
		if Engine.get_physics_frames() % 5:
			propagate()
	
	if not bullet_backend:
		state.integrate_forces()

func propagate():
	if _graph._nodes.size() == 0:
		queue_free()
	# Propagate forces to edges and reset/decay forces
	# detect islands by checking for paths between 
	var graph_update = _graph._edges.size() < _graph._nodes.size()
	var axis_locked = false
	for node_idx in _graph._nodes.keys():
		if not _graph._nodes.has(node_idx):
			continue
		
		# if anchor enabled, while any node is anchored all axis are locked
		if anchor_enabled:
			if _graph._nodes[node_idx].anchor:
				axis_locked = true
		
		var force: float = _graph._nodes[node_idx].force.length_squared()
		_graph._nodes[node_idx].bond -= force
		
		if _graph._nodes[node_idx].bond <= 0:
			var neighbours = _graph.neighbours(node_idx)
			for n in neighbours:
				if not _graph._nodes[n].force.normalized().dot(_graph._nodes[node_idx].force.normalized()) > 0.99:
					_graph.remove_edge(node_idx, n)
					graph_update = true
			
		#_graph._nodes[node_idx].bond = _graph.max_bond
		#_graph._nodes[node_idx].force = Vector3.ZERO
	
	set_anchor_enabled(axis_locked)
	
	if graph_update:
		var subgraphs: Array = _graph.subgraphs()
		
		# sanity check
		var subgraph_node_sum = 0
		for s in subgraphs: subgraph_node_sum += s.size()
		var bitmap: Dictionary = _graph.get_edge_matrix()
		var test = _graph.test_edge_matrix()
		assert(subgraph_node_sum == _graph._nodes.size())
		
		if subgraphs.size() > 1:
			var gidx = 0
			var gsize = 0
			for i in range(subgraphs.size()):
				var sgsize = subgraphs[i].size()
				if sgsize > gsize:
					gsize = sgsize
					gidx = i
			subgraphs.remove(gidx)
			
			for subgraph in subgraphs:
				if subgraph.size() == 1:
					detach_shape(subgraph[0], _graph._nodes[subgraph[0]].force)
				else:
					fracture_subgraph(subgraph)
			
			center_changed = true
			
		elif subgraphs.size() == 1:
			if subgraphs[0].size() == 1: # Single fragment remains, convert to simple rigidbody
				detach_shape(subgraphs[0][0], _graph._nodes[subgraphs[0][0]].force)

func detach_shape(shape_id: int, force: Vector3):
	var new_body = RigidBody.new()
	get_parent().add_child(new_body)
	new_body.global_transform = global_transform
	var colshape = shape_owner_get_owner(shape_id)
	
	if bullet_backend:
		new_body.global_transform = colshape.global_transform
		colshape.transform = Transform.IDENTITY
	
	remove_child(colshape)
	new_body.add_child(colshape)
	colshape.owner = new_body
	
	force = force.normalized() * clamp(force.length(), 0, 100)
	new_body.add_force(force, Vector3.ZERO)
	_graph.remove_node(shape_id)

func fracture_subgraph(subgraph_nodes: Array):
	#Engine.time_scale = 0.05
	# Build a new baked fractured with the shapes in subgraph
	var fracture = get_script().new()
	var subgraph = _graph.remove_subgraph(subgraph_nodes)
	fracture._graph = subgraph
	get_parent().add_child(fracture)
	fracture.global_transform = global_transform
	
	if bullet_backend:
		fracture.global_transform.origin = fracture.to_global(fracture._graph.centroid())
	
	for n in range(subgraph_nodes.size()):
		var colshape = shape_owner_get_owner(subgraph_nodes[n])
		var colshape_global: Vector3 = colshape.global_transform.origin
		remove_child(colshape)
		fracture.add_child(colshape)
		colshape.owner = fracture
		
		if bullet_backend:
			colshape.global_transform.origin = colshape_global
			fracture._graph._nodes[n].position = colshape.transform.origin
	
	var force: Vector3 = fracture._graph._nodes.values()[0].force
	force = force.normalized() * clamp(force.length(), 0, 100)
	fracture.add_force(force, Vector3.ZERO)
	

func recalculate_center_of_mass():
	var shape_globals = []
	for n in get_shape_owners():
		shape_globals.append([n, shape_owner_get_owner(n).global_transform.origin])
	global_transform.origin = to_global(_graph.centroid())
	for n in shape_globals:
		shape_owner_get_owner(n[0]).global_transform.origin = n[1]
		_graph._nodes[n[0]].position = shape_owner_get_owner(n[0]).transform.origin


func mesh_centroid(mesh: Mesh) -> Vector3:
	var verts: Array = mesh.surface_get_arrays(ArrayMesh.ARRAY_VERTEX)
	var sum: Vector3 = Vector3.ZERO
	var vert_count = 0
	for v in range(verts.size()):
		if verts[v] != null:
			for vert in verts[v]:
				sum += vert
				vert_count += 1
	return sum / float(vert_count)


func glue_debug_setup():
	_debug_line_mat.flags_unshaded = true
	_debug_line_mat.flags_use_point_size = true
	_debug_line_mat.albedo_color = Color.white
	_debug_line_mat.vertex_color_use_as_albedo = true
	_debug_line_mat.params_line_width = 4.0
	_debug_line_mat.params_cull_mode = SpatialMaterial.CULL_DISABLED
	_debug_line_mat.flags_no_depth_test = true

	_debug_point_mat.flags_unshaded = true
	_debug_point_mat.flags_use_point_size = true
	_debug_point_mat.albedo_color = Color.white
	_debug_point_mat.vertex_color_use_as_albedo = true
	_debug_point_mat.params_point_size = 10.0
	_debug_point_mat.params_cull_mode = SpatialMaterial.CULL_DISABLED
	_debug_point_mat.flags_no_depth_test = true

	_debug_drawer = ImmediateGeometry.new()
	add_child(_debug_drawer)
	_debug_drawer.clear()

func glue_debug_draw():
	if not Engine.editor_hint and glue_debug_graph and _graph != null:
		_debug_drawer.clear()
		_debug_drawer.set_material_override(_debug_line_mat)
		for x in _graph._edges:
			_debug_drawer.begin(Mesh.PRIMITIVE_LINE_STRIP, null)
			_debug_drawer.set_color(Color.white)
			_debug_drawer.add_vertex(_graph._nodes[x.a].position)
			_debug_drawer.add_vertex(_graph._nodes[x.b].position)
			_debug_drawer.end()
		_debug_drawer.set_material_override(_debug_point_mat)
		for n in _graph._nodes.values():
			_debug_drawer.begin(Mesh.PRIMITIVE_POINTS, null)
			if n.anchor:
				_debug_drawer.set_color(Color(0,0,1))
			else:
				if n.force.length() > 0:
					_debug_drawer.set_color(Color(1,0,0))
				else:
					_debug_drawer.set_color(Color(0,1,0))
			_debug_drawer.add_vertex(n.position)
			_debug_drawer.end()
		if bullet_backend:
			# draw center of mass
			_debug_drawer.begin(Mesh.PRIMITIVE_POINTS, null)
			_debug_drawer.set_color(Color.purple)
			_debug_drawer.add_vertex(Vector3.ZERO)
			_debug_drawer.end()

func graph_test(gclass):
	var g = gclass.new()
	g.add_node(0, Vector3.ZERO)
	g.add_node(1, Vector3.ZERO)
	g.add_node(2, Vector3.ZERO)
	g.add_node(3, Vector3.ZERO)
	g.add_edge(0,1)
	g.add_edge(1,2)
	g.add_edge(2,0)
	g.add_edge(2,3)
	g.remove_edge(2,3)
	g.remove_node(3)
	
	assert(g.adjacent(0, 1))
	print("Node 1 neighbours: ", g.neighbours(1))
	
	print("Graph Centroid: ", g.centroid())
	
	var s = g.subgraphs()
	print("Graph subgraphs: ", s)
	
	print("Subgraph 0 edges: ", g.nodes_edges(s[0]))
	
	var sg = g.remove_subgraph(s[0])
	print("Remove subgraph 0: ", sg)
	print(sg._nodes)
	print(sg._edges)
	
	print(g._nodes.keys())
	print(g._edges)
	print("Graph Test Done")
	
