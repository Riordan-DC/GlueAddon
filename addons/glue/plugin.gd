tool
extends EditorPlugin

var _glue_button: Button
#var _fracture_button: Button
var selected_object: Object

const BakedFracture = preload("res://addons/glue/scripts/BakedFracture.gd")
const GraphGizmo = preload("res://addons/glue/scripts/GraphGizmoPlugin.gd")
const BakedFractureCPP = preload("res://addons/glue/gdnative/bakedfracture_class.gdns")
const GGraph = preload("res://addons/glue/gdnative/ggraph_class.gdns")
const GlueUtils = preload("res://addons/glue/scripts/GlueUtils.gd")

var gizmo_plugin = GraphGizmo.new()


func handles(obj):
	if obj is BakedFracture or "_graph" in obj:
		_glue_button.visible = true
#		_fracture_button.visible = true
	else:
		_glue_button.visible = false
#		_fracture_button.visible = false
	
	return obj != null and obj is BakedFracture or  "_graph" in obj

func edit(object: Object) -> void:
	selected_object = object
	

func _enter_tree() -> void:
	#add_custom_type("BakedFractureGD", "RigidBody", BakedFracture, null)
	add_custom_type("BakedFracture", "RigidBody", BakedFractureCPP, null)
	
	_glue_button = Button.new()
	_glue_button.flat = true
	_glue_button.text = "Glue"
	_glue_button.hide()
	_glue_button.connect("pressed", self, "_on_glue_button_pressed")
	add_control_to_container(EditorPlugin.CONTAINER_SPATIAL_EDITOR_MENU, _glue_button)
	
#	_fracture_button = Button.new()
#	_fracture_button.flat = true
#	_fracture_button.text = "Fracture"
#	_fracture_button.hide()
#	_fracture_button.connect("pressed", self, "_on_fracture_button_pressed")
#	add_control_to_container(EditorPlugin.CONTAINER_SPATIAL_EDITOR_MENU, _fracture_button)
	
	var base_control = get_editor_interface().get_base_control()
	
	# Gizmo
	add_spatial_gizmo_plugin(gizmo_plugin)

func _exit_tree() -> void:
	remove_control_from_container(EditorPlugin.CONTAINER_SPATIAL_EDITOR_MENU, _glue_button)
	_glue_button.queue_free()
	
	# Gizmo
	remove_spatial_gizmo_plugin(gizmo_plugin)


func _on_glue_button_pressed() -> void:
	if selected_object:
		if selected_object.has_method("glue"):
			print("has glue method")
			selected_object.glue()
		else:
			print("Glue Object")
			var parent = null
			if selected_object.get_parent():
				parent = selected_object.get_parent()
			GlueUtils.glue(selected_object, get_tree().edited_scene_root, get_script().get_path().get_base_dir(), parent)

func _on_fracture_button_pressed() -> void:
	printerr("IMPLIMENT FRACTURE")
	# Using mesh AABB make voronoi cells
	
	# Make CSG of mesh and for each voronoi cell get intersecting mesh
