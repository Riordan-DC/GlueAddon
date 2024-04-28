# GlueGizmoPlugin.gd
extends EditorSpatialGizmoPlugin

const BakedFracture = preload("res://addons/glue/scripts/BakedFracture.gd")
const BakedFractureCPP = preload("res://addons/glue/gdnative/bakedfracture_class.gdns")
const GraphGizmo = preload("res://addons/glue/scripts/GraphGizmo.gd")

func get_name():
	return "GlueNode"

func has_gizmo(spatial):
	return spatial is BakedFracture or "_graph" in spatial

func _init():
	var mat = SpatialMaterial.new()
	mat.albedo_color = Color(0,1,0)
	mat.flags_no_depth_test = true
	mat.flags_unshaded = true
	#create_material("main", Color(0, 1, 0), false, true, false)
	add_material("main", mat)
	create_handle_material("handles")
	var anchor_texture = load("res://addons/glue/anchor_icon.png")
	create_handle_material("anchor", false, anchor_texture)

func create_gizmo(spatial):
	if spatial is BakedFracture or "_graph" in spatial:
		return GraphGizmo.new()
	else:
		return null

#func redraw(gizmo):
#	gizmo.clear()
#
#	var spatial = gizmo.get_spatial_node()
#
#	var lines = PoolVector3Array()
#
#	lines.push_back(Vector3(0, 1, 0))
#	lines.push_back(Vector3(0, spatial.my_custom_value, 0))
#
#	var handles = PoolVector3Array()
#
#	handles.push_back(Vector3(0, 1, 0))
#	handles.push_back(Vector3(0, spatial.my_custom_value, 0))
#
#	gizmo.add_lines(lines, get_material("main", gizmo), false)
#	gizmo.add_handles(handles, get_material("handles", gizmo))
