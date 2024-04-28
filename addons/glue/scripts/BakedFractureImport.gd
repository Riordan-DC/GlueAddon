tool
extends EditorScenePostImport

# Converts a flat heirachy of meshes to a baked fracture
# Its spatial parent will become a BakedFracture
# Its mesh children will become collision shapes

# TODO:
# If the mesh name contains the string '-bb' the bounding
# box will be used which runs faster in the engine.

# WARNING: Building of the glue graph still has to be done in the editor or when running. This is because
# we need a physics world to do the calculations and this import tool script doesnt have a world.

const fracture_physics_layer = (1 << 7)


func post_import(scene):
	var BakedFracture = load("res://addons/glue/gdnative/bakedfracture_class.gdns")
	var GGraph = load("res://addons/glue/gdnative/ggraph_class.gdns")
	var GlueUtils = load("res://addons/glue/scripts/GlueUtils.gd")
	
	var fractures = []
	var queue = [scene]
	
	while not queue.empty():
		var node = queue.pop_front()
		
		if node:
			queue.append_array(node.get_children())
			
			if "-fracture" in node.name:
				fractures.append(node)
	
	for node in fractures:
		print("Fracture: ", node.name)
		var fracture = RigidBody.new()
		fracture.collision_layer = fracture_physics_layer
		fracture.collision_mask |= fracture_physics_layer
		
		fracture.set_script(BakedFracture)
		fracture._graph = GGraph.new()
		scene.add_child(fracture)
		fracture.set_owner(scene)
		fracture.transform = node.transform
		fracture.anchor_enabled = true
		
		var cells = []
		for cell in node.get_children():
			if cell is MeshInstance:
				cells.append(cell)
		
		for i in range(cells.size()):
#			print("Cell (%d/%d)" % [i+1, cells.size()])
			var mesh = cells[i].mesh as Mesh
			
			# Discard invalid meshes, for example, a single triangle cannot become a collision mesh 
			var mesh_faces = mesh.get_faces().size()
			var mesh_size = mesh.get_aabb().size.length()
			
			var valid_mesh = mesh_faces > 3 && mesh_size > 1.0
			
			if valid_mesh:
#				print(mesh.get_faces().size(), mesh.get_aabb().size.length())
	#			print(mesh.get_surface_count())
				var shape = mesh.create_convex_shape(true, false)
				
				var collider = CollisionShape.new()
				fracture.add_child(collider)
				collider.set_owner(scene)
				collider.shape = shape
				collider.transform = cells[i].transform
				
				node.remove_child(cells[i])
				collider.add_child(cells[i])
				cells[i].set_owner(scene)
				cells[i].transform = Transform.IDENTITY
			else:
				node.remove_child(cells[i])
				cells[i].queue_free()
		
		node.get_parent().remove_child(node)
		fracture.add_child(node)
		node.set_owner(scene)
		node.transform = Transform.IDENTITY
	
	
	return scene

