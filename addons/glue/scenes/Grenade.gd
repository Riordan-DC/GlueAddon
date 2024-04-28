extends RigidBody

export(bool) var active = false
export(float) var fuse = 5.0
export(float) var force = 30.0

var timer
onready var explosion_area = $Area
onready var explosion_shape = $Area/CollisionShape

func _ready():
	timer = Timer.new()
	timer.connect("timeout", self, "explode", [explosion_shape])
	add_child(timer)
	
	if active:
		start_fuse()

func explode(_explosion_shape: CollisionShape):
	if is_inside_tree():
		var dss = get_world().direct_space_state
		var q = PhysicsShapeQueryParameters.new()
		q.shape_rid = _explosion_shape.shape.get_rid()
		q.transform = global_transform
		q.collision_mask = (1 << 7) | (1 << 0)
		q.exclude = [self]
	
		var results = dss.intersect_shape(q, 22)
		var fractures = []
		
		# Our first loop just gets all the shape ids! We must get these before we run detach because it
		# will re-order the ids.
		var body_shapes = {}
		
		for result in results:
			var body = result["collider"]
			var shape = result["shape"]
			
			q.exclude.append(body)
#			var impulse_direction = global_transform.origin.direction_to(body.global_transform.origin) * force
			
			if body is RigidBody:
				var ons = body.get_shape_owners()
				if ons.size() > shape:
					shape = ons[shape]
					if body_shapes.has(body):
						body_shapes[body].append(shape)
					else:
						body_shapes[body] = [shape]
		
		for body in body_shapes.keys():
			for shape in body_shapes[body]:
				if body is RigidBody:
					if body.has_method("detach_shape"):
						# REVISIT THIS LINE. MIGHT HAVE TO USE UNMAPPED SHAPE
						var shape_position = body.to_global(body.shape_owner_get_transform(shape).origin)
						
						var impulse_direction = _explosion_shape.global_transform.origin.direction_to(shape_position) * force
						body.detach_shape(shape, impulse_direction) 
						
						if not fractures.has(body):
							fractures.append(body)
					else: # Apply force to rigidbody
						var impulse_direction = _explosion_shape.global_transform.origin.direction_to(body.global_transform.origin) * force
						body.apply_central_impulse(impulse_direction)
					
					if body.has_method("hit"):
						body.call("hit")
			
#		for fracture in fractures:
#			fracture.propagate(true)
#			fracture.connectivity_changed = true
#			fracture.emit_signal("fractured")
		
#		yield(get_tree(), "idle_frame")
#		# Apply impulse forces again
#		results = dss.intersect_shape(q, 12)
#		for result in results:
#			var body = result["collider"]
##			var shape = result["shape"]
#			var impulse_direction = global_transform.origin.direction_to(body.global_transform.origin) * force
#			if body is RigidBody:
#				# Apply force to rigidbody
#				body.apply_central_impulse(impulse_direction)
		
		queue_free()

func start_fuse():
	timer.start(fuse)
