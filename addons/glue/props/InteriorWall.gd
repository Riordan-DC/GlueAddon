extends Node3D

func _ready():
	$InteriorWall.connect("fractured", self.fractured)
	$InteriorWall.visible = false
	
	#for child in $InteriorWall.get_children():
		#if child is CollisionShape3D:
			#child.shape.margin = 0.01

func fractured():
	$InteriorWallWhole.visible = false
	$InteriorWall.visible = true
