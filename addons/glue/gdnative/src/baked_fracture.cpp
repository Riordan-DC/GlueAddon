/* baked_fracture.h */
#include "baked_fracture.h"


BakedFracture::BakedFracture() {

}

BakedFracture::~BakedFracture() {

}

void BakedFracture::_init() {

}

// void BakedFracture::set_debug_graph(bool value) {
	// glue_debug_graph = value;
	// if (_debug_drawer != nullptr)
	// 	_debug_drawer->set_visible(glue_debug_graph);
// }

void BakedFracture::_ready() {
	Variant engine_name = ProjectSettings::get_singleton()->get("physics/3d/physics_engine");
	if (engine_name.get_type() == Variant::STRING) {
		if (engine_name == "JoltPhysics3D") {
			//godot::UtilityFunctions::print(engine_name);
			jolt_backend = true;
			set_anchor_enabled(anchor_enabled);
		}
	}
	// test
	jolt_backend = true;

	set_freeze_mode(RigidBody3D::FREEZE_MODE_KINEMATIC);
	set_process(true);
	set_angular_damp(0.000001f);
	set_contact_monitor(true);

	velocity_inital = get_linear_velocity();
	
	if (get_max_contacts_reported() == 0)
		set_max_contacts_reported(24);
	set_use_custom_integrator(!bullet_backend);

	if (!Engine::get_singleton()->is_editor_hint()) {
		if (_graph != nullptr) {
			if (glue_debug_graph) {
				glue_debug_setup();
				glue_debug_draw();
			}

			//ResourceLoader* loader = ResourceLoader::get_singleton();
			//_graph = loader->load(_graph->get_path());

			_graph = _graph->duplicate(true);
			if (_graph->_edges.size() > 30) {
				_graph->setup_edge_matrix();
			}

			// set max bond
			// TODO: Check here
			Array keys = _graph->_nodes.keys();
			for (int i = 0; i < keys.size(); i++) {
				Dictionary node = _graph->_nodes[keys[i]];
				node["bond"] = glue_strength;
			}
		}
		recalculate_center_of_mass();
	}
}

void BakedFracture::_process(double p_delta) {
	update();
}

void BakedFracture::update() {
	if (glue_debug_graph) {
		if (_debug_drawer != nullptr) {
			if (Engine::get_singleton()->get_process_frames() % 20 == 0) {
				glue_debug_draw();
			}
		} else {
			glue_debug_setup();
		}
	}

	if (bullet_backend) {
		if (center_changed && !Engine::get_singleton()->is_in_physics_frame()) {
			recalculate_center_of_mass();
			center_changed = false;
		}
	}

	if (connectivity_changed) {
		propagate(true);
		connectivity_changed = false;
	}
}

void BakedFracture::set_anchor_enabled(bool value) {
	anchor_enabled = value;
	// TODO: Check up on this
	if (true || ProjectSettings::get_singleton()->get("physics/3d/physics_engine") == "JoltPhysics3D") {
		jolt_backend = true;
		set_freeze_enabled(value);
	}
	else {
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_LINEAR_X, value);
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_LINEAR_Y, value);
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_LINEAR_Z, value);
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_ANGULAR_X, value);
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_ANGULAR_Y, value);
		set_axis_lock(PhysicsServer3D::BodyAxis::BODY_AXIS_ANGULAR_Z, value);
	}
}

void BakedFracture::_integrate_forces(PhysicsDirectBodyState3D* state) {
	if (!state->is_sleeping()) {
		int64_t contact_count = state->get_contact_count();
		Array shape_owners = get_shape_owners();
		for (int64_t contact = 0; contact < contact_count; contact++) {
			int64_t shape_id = state->get_contact_local_shape(contact);
			if (shape_id >= shape_owners.size() || shape_id == -1) {
				continue; // shape_id out of bounds. This rarely happens.
			}
			//shape_id = shape_find_owner(shape_id);
			// indirect the index through the shape owners array because internally CollisionShape3D reorganises shape ids.
			shape_id = shape_owners[shape_id];

			Dictionary node = _graph->_nodes[shape_id];
			if (anchor_static) {
				if (node["anchor"]) {
					continue;
				}
			}

			Vector3 contact_normal = state->get_contact_local_normal(contact);
			Object* collider_object = state->get_contact_collider_object(contact);
			RigidBody3D* rigid_body = Object::cast_to<RigidBody3D>(collider_object);

			// delta V = v_f - v_i
			// v_f = velocity final (state->get_linear_velocity())
			// v_i = velocity inital (get_linear_velocity()) ?? assumption: get_linear_velocity() is older by 1 frame than v_f
			Vector3 speed_difference = get_linear_velocity() + get_angular_velocity();
			velocity_inital = speed_difference;

			//speed_difference -= state->get_contact_collider_velocity_at_position(contact);
			
			float mass = get_mass();
			if (rigid_body != nullptr) {
				speed_difference -= (rigid_body->get_linear_velocity() + rigid_body->get_angular_velocity());
				mass += rigid_body->get_mass();
			}

			Vector3 vel = Vector3();
			if (state->get_step() > 0.0) {
				vel = speed_difference; // state->get_step();
			}

			Vector3 force_vector = -contact_normal * vel * mass * state->get_step(); //state->get_contact_impulse(contact);
			real_t force_power = force_vector.length() * Engine::get_singleton()->get_time_scale();
			
			// TODO: an anchor is true while it contacts its anchor object
			
			if (force_power > force_threshold) {
				node["force"] = (Vector3)node["force"] + force_vector;
				//Godot::print(String("Node (") + RigidBody3D->get_name() + ")FORCED: " + String::num_int64(shape_id) + " [" + String::num_real(force_power) + "]");
			}
		}

		//uint64_t begin = OS::get_singleton()->get_ticks_usec();

		propagate(false);

		//uint64_t end = OS::get_singleton()->get_ticks_usec();
		//Godot::print(String("propagate microseconds: ") + String::num_int64(end - begin));
	}

	// TODO: Revisit this error
	if (true) { //!bullet_backend) {
		state->integrate_forces();
	}
}

void BakedFracture::propagate(bool force_update) {
	if (_graph->_nodes.size() == 0) {
		queue_free();
	}

	// Propagate forces to edges and reset/decay forces
	// Detect islands by searching connectivity of each node
	bool graph_update = (_graph->_edges.size()+1) < _graph->_nodes.size() || force_update;
	bool axis_locked = false;
	Array keys = _graph->_nodes.keys();
	for (int node_idx = 0; node_idx < keys.size(); node_idx++) {
		int node_key = keys[node_idx];
		if (!_graph->_nodes.has(node_key)) {
			continue;
		}
		
		Dictionary node = _graph->_nodes[node_key];
		// if anchor enabled, while any node is anchored all axis are locked
		if (anchor_enabled) {
			if (node["anchor"])
				axis_locked = true;
		}
		
		float force = ((Vector3)node["force"]).length_squared();
		node["bond"] = (float)node["bond"] - force;
		
		// TODO: Return here and consider commented lines
		if ((float)node["bond"] <= 0.0) {
			PackedInt32Array neighbours = _graph->neighbours(node_key);
			for (int n = 0; n < neighbours.size(); n++) {
				int neighbor = neighbours[n];
				Dictionary neighbor_node = _graph->_nodes[neighbor];
				Vector3 neighbour_force_dir = ((Vector3)neighbor_node["force"]).normalized();
				Vector3 node_force_dir = ((Vector3)node["force"]).normalized();
				// Detect if the force is not uniform. Relative forces "tear" pieces apart.
				if ((neighbour_force_dir.dot(node_force_dir) < 0.90)) {
					_graph->remove_edge(node_key, neighbor);
					//Godot::print(String("Removing edge: ") + String::num_int64(node_key) + " --> " + String::num_int64(neighbor));
					graph_update = true;
				}
			}
			node["bond"] = glue_strength;
			node["force"] = Vector3(0,0,0);
		}
	}
	
	set_anchor_enabled(axis_locked);
	
	if (graph_update) {
		//uint64_t begin = OS::get_singleton()->get_ticks_usec();
		int current_node_count = _graph->_nodes.size();
		Array subgraphs = _graph->subgraphs();
		//uint64_t end = OS::get_singleton()->get_ticks_usec();
		//Godot::print(String("Subgraphs: ") + String::num_int64(subgraphs.size()) + " (" + String::num_int64(_graph->_nodes.size()) + ")" + "[" + String::num_int64(_graph->_edges.size()) + "]" + "subgraphs milliseconds: " + String::num_real((double)(end - begin) / 1000.0f));
		
		// sanity check
		/*
		int64_t subgraph_node_sum = 0;
		for (int i = 0; i < subgraphs.size(); i++) {
			subgraph_node_sum += ((Array)subgraphs[i]).size();
		}
		assert(subgraph_node_sum == _graph->_nodes.size());
		*/

		//return;

		if (subgraphs.size() > 1) {
			// the max subgraph will remain this baked fracture
			int gidx = 0;
			int gsize = 0;
			for (int i = 0; i < subgraphs.size(); i++) {
				Array ar = subgraphs[i];
				int sgsize = ar.size();
				if (sgsize > gsize) {
					gsize = sgsize;
					gidx = i;
				}
			}
			Array ar = subgraphs[gidx];
			if (ar.size() == 1) {
				// Single fragment remains, convert to simple RigidBody3D
				Dictionary node = _graph->_nodes[ar[0]];
				detach_shape( ar[0], node["force"]);
			}
			subgraphs.remove_at(gidx);

			for (int i = 0; i < subgraphs.size(); i++) {
				Array subgraph = subgraphs[i];
				if (subgraph.size() == 1) {
					Dictionary node = _graph->_nodes[subgraph[0]];
					detach_shape(subgraph[0], node["force"]);
				} else {
					fracture_subgraph(subgraph);
				}
			}
			center_changed = true;
			emit_signal("fractured");
		} else if (subgraphs.size() == 1) {
			Array ar = subgraphs[0];
			if (ar.size() == 1) {
				// Single fragment remains, convert to simple RigidBody3D
				Dictionary node = _graph->_nodes[ar[0]];
				detach_shape(ar[0], node["force"]);
			}

			if (ar.size() != current_node_count) {
				center_changed = true;
			}
			emit_signal("fractured");
		}
	}
	if (_graph->_nodes.size() == 0) {
		queue_free();
	}
}

void BakedFracture::detach_shape(int shape_id, Vector3 force) {
	/* 
	WARNING: Massive issue with this function is that it removes shapes (duh).
	Why? Any function that removes shapes seems to shift the shape_ids and means if in a single look you run this 
	function multiple times after the first call the shape_ids are all outdated!
	For example: You want to detatch shape id [3,4]. You should FIRST get the mapping for all shapes ids. Now they are SAFE
	and SECURE.	THEN, loop through each shape and run detatch. :))
	*/
	if (!_graph->_nodes.has(shape_id)) return;
	if (_graph->_nodes.size() == 1) {
		center_changed = true;	
		return;
	}

	RigidBody3D* new_body = memnew(RigidBody3D);
	new_body->set_angular_velocity(get_angular_velocity());
	new_body->set_linear_velocity(get_linear_velocity());

	new_body->set_angular_damp(get_angular_damp());
	new_body->set_linear_damp(get_linear_damp());

	new_body->set_collision_layer(get_collision_layer());
	new_body->set_collision_mask(get_collision_mask());

	if (piece_script != nullptr) {
		new_body->set_script(*piece_script);
	}

	if (group != "") {
		new_body->add_to_group(group);
	}

	if (piece_parent != nullptr) {
		piece_parent->add_child(new_body);
	} else {
		get_parent()->add_child(new_body);
	}
	new_body->set_global_transform(get_global_transform());
	CollisionShape3D* colshape = Object::cast_to<CollisionShape3D>(shape_owner_get_owner(shape_id));
	
	if (colshape == nullptr) {
		return;
	}

	if (bullet_backend) {
		new_body->set_global_transform(colshape->get_global_transform());
		colshape->set_transform(Transform3D(Basis(Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1))));
	}

	remove_child(colshape);
	new_body->add_child(colshape);
	colshape->set_owner(new_body);
	
	//force = force.normalized() * Math::clamp(force.length(), 0.0f, 100.0f);
	_graph->remove_node(shape_id);	
	new_body->apply_impulse(Vector3(), force);
	connectivity_changed = true;
	center_changed = true;
}

void BakedFracture::fracture_subgraph(Array subgraph_nodes) {
	// Build a new baked fractured with the shapes in subgraph
	BakedFracture* fracture = memnew(BakedFracture);

	if (group != "") {
		fracture->add_to_group(group);
	}

	Ref<Ggraph> subgraph = Ref<Ggraph>(_graph->remove_subgraph(subgraph_nodes));

	// Note: unsure of if the fracture should inherit the velocity 
	fracture->set_angular_velocity(get_angular_velocity());
	fracture->set_linear_velocity(get_linear_velocity());

	fracture->set_collision_layer(get_collision_layer());
	fracture->set_collision_mask(get_collision_mask());
	
	fracture->_graph = subgraph;
	fracture->anchor_enabled = anchor_enabled;
	fracture->anchor_static = anchor_static;
	fracture->glue_debug_graph = glue_debug_graph;
	
	if (piece_parent != nullptr) {
		piece_parent->add_child(fracture);
	} else {
		get_parent()->add_child(fracture);
	}
	
	fracture->set_global_transform(get_global_transform());
	
	if (bullet_backend && jolt_backend == false) {
		Transform3D t = fracture->get_global_transform();
		t.origin = fracture->to_global(fracture->_graph->centroid());
		fracture->set_global_transform(t);
	}
	
	for (int n = 0; n < subgraph_nodes.size(); n++) {
		CollisionShape3D* colshape = Object::cast_to<CollisionShape3D>(shape_owner_get_owner(subgraph_nodes[n]));
		Vector3 colshape_global = colshape->get_global_transform().origin;
		remove_child(colshape);
		fracture->add_child(colshape);
		colshape->set_owner(fracture);
		
		if (bullet_backend) {
			Transform3D t = colshape->get_global_transform();
			t.origin = colshape_global;
			colshape->set_global_transform(t);

			Dictionary d = fracture->_graph->_nodes[n];
			d["position"] = colshape->get_transform().origin;
		}
	}

	Dictionary n = fracture->_graph->_nodes.values()[0];
	Vector3 force = n["force"];
	//force = force.normalized() * Math::clamp(force.length(), 0.0f, 100.0f);
	fracture->apply_central_impulse(force);
}

void BakedFracture::recalculate_center_of_mass() {
	return;
	// This should not be needed in Godot 4 where CoM is computed automatically
	/*
	Array shape_globals;
	Array shape_owners = get_shape_owners();
	for (int n = 0; n < shape_owners.size(); n++) {
		Array pair;
		pair.append(shape_owners[n]);
		pair.append(Object::cast_to<CollisionShape3D>(shape_owner_get_owner(shape_owners[n]))->get_global_transform().origin);
		shape_globals.append(pair);
	}

	Transform3D t = get_global_transform();
	t.origin = to_global(_graph->centroid());
	set_global_transform(t);
	for (int n = 0; n < shape_globals.size(); n++) {
		Array pair = shape_globals[n];
		int shaper_owner = pair[0];
		CollisionShape3D* colshape = Object::cast_to<CollisionShape3D>(shape_owner_get_owner(shaper_owner));
		Transform3D t = colshape->get_global_transform();
		t.origin = pair[1];
		colshape->set_global_transform(t);
		Dictionary node = _graph->_nodes[pair[0]];
		node["position"] = colshape->get_transform().origin;
	}
	*/
}

void BakedFracture::glue_debug_setup() {
	_debug_line_mat = memnew(StandardMaterial3D);
	_debug_line_mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	_debug_line_mat->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	_debug_line_mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	_debug_line_mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	
	_debug_point_mat = memnew(StandardMaterial3D);
	_debug_point_mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	_debug_point_mat->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	_debug_point_mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	_debug_point_mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	_debug_point_mat->set_flag(BaseMaterial3D::FLAG_USE_POINT_SIZE, true);
	_debug_point_mat->set_albedo(Color(1.0,1.0,1.0,1.0));
	_debug_point_mat->set_point_size(10.0);
	_debug_point_mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	
	_debug_drawer = memnew(MeshInstance3D);
	add_child(_debug_drawer);
	Ref<ImmediateMesh> mesh = memnew(ImmediateMesh);
	_debug_drawer->set_mesh(mesh);

	_debug_drawer_points = memnew(MeshInstance3D);
	add_child(_debug_drawer_points);
	Ref<ImmediateMesh> point_mesh = memnew(ImmediateMesh);
	_debug_drawer_points->set_mesh(point_mesh);

	//_debug_drawer->set_material_override(_debug_line_mat);
}

void BakedFracture::glue_debug_draw() {
	if (!Engine::get_singleton()->is_editor_hint() && glue_debug_graph && _graph != nullptr) {
		{
			// Draw edges
			Ref<ImmediateMesh> im = _debug_drawer->get_mesh();
			if (im != nullptr) {
				im->clear_surfaces();
				for (int i = 0; i < _graph->_edges.size(); i++) {
					Dictionary x = _graph->_edges[i];
					Dictionary node1 = _graph->_nodes[x["a"]];
					Dictionary node2 = _graph->_nodes[x["b"]];
					im->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINES, _debug_line_mat);
					im->surface_set_color(Color(1.0, 1.0, 1.0, 1.0));
					im->surface_add_vertex(node1["position"]);
					im->surface_add_vertex(node2["position"]);
					im->surface_end();
				}
			}
		}
		
		{
			// Draw nodes
			Ref<ImmediateMesh> im = _debug_drawer_points->get_mesh();
			if (im != nullptr) {
				im->clear_surfaces();
				Array keys = _graph->_nodes.keys();
				for (int x = 0; x < keys.size(); x++) {
					Variant key = keys[x];
					Dictionary n = _graph->_nodes[key];
					im->surface_begin(Mesh::PrimitiveType::PRIMITIVE_POINTS, _debug_point_mat);
					im->surface_add_vertex(n["position"]);
					if (n["anchor"]) {
						im->surface_set_color(Color(0, 0, 1));
					}
					else {
						if (((Vector3)n["force"]).length() > 0) {
							im->surface_set_color(Color(1, 0, 0));
						}
						else {
							im->surface_set_color(Color(0, 1, 0));
						}
					}
					im->surface_end();
				}
			}
		}
	}
	
	// 	if (bullet_backend) {
	// 		// draw center of mass
	// 		_debug_drawer->begin(Mesh::PrimitiveType::PRIMITIVE_POINTS);
	// 		_debug_drawer->set_color(Color(0.5, 0.0, 1.0, 1.0));
	// 		_debug_drawer->add_vertex(Vector3());
	// 		_debug_drawer->end();
	// 	}
	// }
}

void BakedFracture::set_graph(Ref<Ggraph> graph) {
	//ResourceLoader* loader = ResourceLoader::get_singleton();
	_graph = graph; //loader->load(graph->get_path());
}

Ref<Ggraph> BakedFracture::get_graph() {
	return _graph;
}

void BakedFracture::_bind_methods() {
	// Functions
	//ClassDB::bind_method("glue_set_debug_graph", &BakedFracture::glue_set_debug_graph);
	// ClassDB::bind_method(D_METHOD("_ready"), &BakedFracture::_ready);
	// ClassDB::bind_method(D_METHOD("_process"), &BakedFracture::_process);
	// ClassDB::bind_method(D_METHOD("_integrate_forces"), &BakedFracture::_integrate_forces);
	ClassDB::bind_method(D_METHOD("update"), &BakedFracture::update);
	ClassDB::bind_method(D_METHOD("propagate", "force_update"), &BakedFracture::propagate);
	ClassDB::bind_method(D_METHOD("detach_shape", "shape_id", "force"), &BakedFracture::detach_shape);
	ClassDB::bind_method(D_METHOD("fracture_subgraph", "nodes"), &BakedFracture::fracture_subgraph);
	ClassDB::bind_method(D_METHOD("recalculate_center_of_mass"), &BakedFracture::recalculate_center_of_mass);
	ClassDB::bind_method(D_METHOD("glue_debug_setup"), &BakedFracture::glue_debug_setup);
	ClassDB::bind_method(D_METHOD("glue_debug_draw"), &BakedFracture::glue_debug_draw);


	// Properties
	ClassDB::bind_method(D_METHOD("set_glue_debug_graph", "value"), &BakedFracture::set_glue_debug_graph);
	ClassDB::bind_method(D_METHOD("get_glue_debug_graph"), &BakedFracture::get_glue_debug_graph);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "glue_debug_graph"), "set_glue_debug_graph", "get_glue_debug_graph");

	ClassDB::bind_method(D_METHOD("set_glue_margin", "value"), &BakedFracture::set_glue_margin);
	ClassDB::bind_method(D_METHOD("get_glue_margin"), &BakedFracture::get_glue_margin);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glue_margin"), "set_glue_margin", "get_glue_margin");

	ClassDB::bind_method(D_METHOD("set_glue_strength", "value"), &BakedFracture::set_glue_strength);
	ClassDB::bind_method(D_METHOD("get_glue_strength"), &BakedFracture::get_glue_strength);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "glue_strength"), "set_glue_strength", "get_glue_strength");

	ClassDB::bind_method(D_METHOD("set_force_threshold", "value"), &BakedFracture::set_force_threshold);
	ClassDB::bind_method(D_METHOD("get_force_threshold"), &BakedFracture::get_force_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "force_threshold"), "set_force_threshold", "get_force_threshold");

	ClassDB::bind_method(D_METHOD("set_anchor_enabled", "value"), &BakedFracture::set_anchor_enabled);
	ClassDB::bind_method(D_METHOD("get_anchor_enabled"), &BakedFracture::get_anchor_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anchor_enabled"), "set_anchor_enabled", "get_anchor_enabled");

	ClassDB::bind_method(D_METHOD("set_anchor_static", "value"), &BakedFracture::set_anchor_static);
	ClassDB::bind_method(D_METHOD("get_anchor_static"), &BakedFracture::get_anchor_static);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anchor_static"), "set_anchor_static", "get_anchor_static");

	ClassDB::bind_method(D_METHOD("set_piece_script", "value"), &BakedFracture::set_piece_script);
	ClassDB::bind_method(D_METHOD("get_piece_script"), &BakedFracture::get_piece_script);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "piece_script"), "set_piece_script", "get_piece_script");

	ClassDB::bind_method(D_METHOD("set_group", "value"), &BakedFracture::set_group);
	ClassDB::bind_method(D_METHOD("get_group"), &BakedFracture::get_group);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "group"), "set_group", "get_group");

	ClassDB::bind_method(D_METHOD("set_piece_parent", "value"), &BakedFracture::set_piece_parent);
	ClassDB::bind_method(D_METHOD("get_piece_parent"), &BakedFracture::get_piece_parent);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "piece_parent"), "set_piece_parent", "get_piece_parent");	

	ClassDB::bind_method(D_METHOD("set_graph", "value"), &BakedFracture::set_graph);
	ClassDB::bind_method(D_METHOD("get_graph"), &BakedFracture::get_graph);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "_graph"), "set_graph", "get_graph");

	ClassDB::bind_method(D_METHOD("set_connectivity_changed", "value"), &BakedFracture::set_connectivity_changed);
	ClassDB::bind_method(D_METHOD("get_connectivity_changed"), &BakedFracture::get_connectivity_changed);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "connectivity_changed"), "set_connectivity_changed", "get_connectivity_changed");

	// Signals
	ADD_SIGNAL(MethodInfo("fractured"));
}