extends Spatial

var mouse_relative = Vector2.ZERO

var picked_obj: RigidBody = null
var picked_pos = null

onready var Camera3D = $Camera
onready var Ray = $Camera/RayCast
onready var Marker = $Camera/marker
#const Bullet = preload("res://addons/glue/scenes/DebugBullet.tscn") #preload("res://items/Grenade/Grenade.tscn")#
const Bullet = preload("res://addons/glue/scenes/Grenade.tscn")

var snap_velocity = 20.0
export(float, 0.002, 0.01) var MOUSE_SENSITIVITY := 0.001 # radians/pixel
var MoveDirection = Vector3.ZERO
var Velocity = Vector3.ZERO
var MAX_SPEED = 10.0
var ACCELERATION = 10.0

var grenade_spawned = false

var bullets = []

func _ready():
	#Engine.time_scale = 0.1
	Ray.enabled = true
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
	pause_mode = Node.PAUSE_MODE_PROCESS
	ProjectSettings.set("physics/3d/default_linear_damp", 1.0)
	ProjectSettings.set("physics/3d/default_angular_damp", 1.0)
#	PhysicsServer.

func _input(event):
	#mouse_pos = get_viewport().get_mouse_position()
	if event is InputEventMouseMotion:
		update_camera(event)
	
	if event is InputEventKey:
		if event.scancode == KEY_R:
			get_tree().reload_current_scene()
	
		if event.pressed and event.scancode == KEY_F12:
			OS.window_fullscreen = !OS.window_fullscreen

	if event is InputEventMouseButton:
		if event.button_index == BUTTON_RIGHT and grenade_spawned == false:
			grenade_spawned = true
			var new_bullet = Bullet.instance()
			get_parent().add_child(new_bullet)
			new_bullet.mode = RigidBody.MODE_RIGID
			new_bullet.global_transform.origin = global_transform.origin
			var direction: Vector3 = -new_bullet.to_local(Camera3D.global_transform.basis.z)
			new_bullet.look_at(direction, Vector3.UP)
			new_bullet.continuous_cd = true
			new_bullet.linear_velocity = -Camera3D.global_transform.basis.z * 30.0
		
		if event.pressed == false:
			grenade_spawned = false
		#yield(get_tree().create_timer(3.0), "timeout")
		#new_bullet.queue_free()

func _unhandled_input(event):
	if event is InputEventMouseMotion and Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
		update_camera(event)
#	if event is InputEventKey:
#		if event.is_action_pressed("ui_cancel"):
#			get_tree().paused = !get_tree().paused

func update_camera(event):
	rotate_y(-event.relative.x * MOUSE_SENSITIVITY)
	Camera3D.rotate_x(-event.relative.y * MOUSE_SENSITIVITY)
	Camera3D.rotation.x = clamp(Camera3D.rotation.x, -deg2rad(75), deg2rad(80))

func update_input():
	# Movement
	MoveDirection = Vector3()
	var cam_xform = Camera3D.get_global_transform()
	var input_dir = Vector2()
	# desired move in camera direction
	if Input.is_physical_key_pressed(KEY_W):
		input_dir.y += 1
	if Input.is_physical_key_pressed(KEY_S):
		input_dir.y -= 1
	if Input.is_physical_key_pressed(KEY_A):
		input_dir.x -= 1
	if Input.is_physical_key_pressed(KEY_D):
		input_dir.x += 1
	input_dir = input_dir.normalized()
	# Basis vectors are already normalized.
	MoveDirection += -cam_xform.basis.z * input_dir.y
	MoveDirection += cam_xform.basis.x * input_dir.x


func _process(delta):
	update_input()
	Velocity = Velocity.linear_interpolate(MoveDirection * MAX_SPEED, ACCELERATION * delta)
	global_transform.origin += Velocity * delta
	
	var mouse_pos = get_viewport().get_mouse_position()
	#var mouse_pos = get_viewport().get_mouse_position()
	
	var from = Camera3D.project_ray_origin(mouse_pos)
	var to = from + Camera3D.project_ray_normal(mouse_pos) * 1000
	var space_state = get_world().get_direct_space_state()
	var hit = space_state.intersect_ray(from, to)
	if hit.size() != 0:
		# collider will be the node you hit
		var body = hit.collider
		if body is RigidBody:
			if Input.is_mouse_button_pressed(BUTTON_LEFT):
				picked_obj = body
				picked_pos = picked_obj.global_transform.origin
				
				if body.has_method("detach_shape"):
					var ons = body.get_shape_owners()
					
#					var query = PhysicsShapeQueryParameters.new()
#					query.margin = 0.04
#					query.transform = Transform(Basis.IDENTITY, hit.position)
#					query.exclude = [self]
#					var shape = SphereShape.new()
#					shape.radius = 0.5
#					query.set_shape(shape)
#					var info: Array = space_state.intersect_shape(query, 32)
#					for col in info:
#						if col.collider_id == hit.collider_id:
#							body.detach_shape(ons[col.shape], Vector3.ZERO)
					
					body.detach_shape(ons[hit.shape], Vector3.ZERO); # VITAL INFORMATION: BakedFracture graph nodes cannot be indexed by shape_id but instead indirected through the body's shape owners array.
					body.propagate(true)
			else:
				picked_obj = null
				picked_pos = null
				Marker.visible = false
				
		Marker.global_transform.origin = Ray.get_collision_point()

	
	if picked_obj and Input.is_mouse_button_pressed(BUTTON_LEFT):
		Marker.visible = true
		var zdepth: float = global_transform.origin.distance_to(picked_pos)
		var new_pos: Vector3 = Camera3D.project_position(mouse_pos, zdepth)
		Marker.global_transform.origin = new_pos
		var distance: float = picked_obj.global_transform.origin.distance_to(new_pos)
		var direction: Vector3 = picked_obj.global_transform.origin.direction_to(new_pos)
		picked_obj.add_central_force(direction * distance * snap_velocity)
	
	if Input.is_action_just_pressed("ui_accept"):
		if Engine.time_scale > 0.06:
			Engine.time_scale = 0.05
		else:
			Engine.time_scale = 1.0
	
