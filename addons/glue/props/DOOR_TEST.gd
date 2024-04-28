extends Spatial

onready var fracture = $InteriorWall
onready var facade = $InteriorWallWhole

func _ready():
	fracture.connect("fractured", self, "fractured")
	fracture.visible = false
	fracture.piece_parent = get_parent()
	set_process(false)

func fractured():
	facade.visible = false
	fracture.visible = true

func _process(delta):
	if is_instance_valid(fracture):
		if fracture.anchor_enabled:
			print("VALID")
			rotation.y += 1.0 * delta
		else:
			print("INVALID")
	else:
		print("INVALID")
	#rotation.y += 1.0 * delta
	
	
