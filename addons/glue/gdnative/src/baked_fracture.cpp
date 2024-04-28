/* baked_fracture.h */
#include "baked_fracture.h"


BakedFracture::BakedFracture() {

}

BakedFracture::~BakedFracture() {

}

void BakedFracture::_init() {

}

void BakedFracture::set_debug_graph(bool value) {
	glue_debug_graph = value;
	if (_debug_drawer != nullptr)
		_debug_drawer->set_visible(glue_debug_graph);
}

bool BakedFracture::get_debug_graph() {
	return glue_debug_graph;
}

void BakedFracture::_ready() {
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

void BakedFracture::_process(float p_delta) {
	update();
}

void BakedFracture::update() {
	if (glue_debug_graph) {
		if (_debug_drawer != nullptr) {
			if (Engine::get_singleton()->get_idle_frames() % 20 == 0) {
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
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_LINEAR_X, value);
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_LINEAR_Y, value);
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_LINEAR_Z, value);
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_ANGULAR_X, value);
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_ANGULAR_Y, value);
	set_axis_lock(PhysicsServer::BodyAxis::BODY_AXIS_ANGULAR_Z, value);
}

void BakedFracture::_integrate_forces(PhysicsDirectBodyState* state) {
	if (!state->is_sleeping()) {
		int64_t contact_count = state->get_contact_count();
		Array shape_owners = get_shape_owners();
		for (int64_t contact = 0; contact < contact_count; contact++) {
			int64_t shape_id = state->get_contact_local_shape(contact);
			if (shape_id >= shape_owners.size() || shape_id == -1) {
				continue; // shape_id out of bounds. This rarely happens.
			}
			//shape_id = shape_find_owner(shape_id);
			// indirect the index through the shape owners array because internally CollisionShape reorganises shape ids.
			shape_id = shape_owners[shape_id];

			Dictionary node = _graph->_nodes[shape_id];
			if (anchor_static) {
				if (node["anchor"]) {
					continue;
				}
			}

			Vector3 contact_normal = state->get_contact_local_normal(contact);
			Object* collider_object = state->get_contact_collider_object(contact);
			RigidBody* rigid_body = Object::cast_to<RigidBody>(collider_object);

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

			Vector3 vel = Vector3::ZERO;
			if (state->get_step() > 0.0) {
				vel = speed_difference; // state->get_step();
			}

			Vector3 force_vector = -contact_normal * vel * mass * state->get_step(); //state->get_contact_impulse(contact);
			real_t force_power = force_vector.length() * Engine::get_singleton()->get_time_scale();
			
			// TODO: an anchor is true while it contacts its anchor object
			
			if (force_power > force_threshold) {
				node["force"] = (Vector3)node["force"] + force_vector;
				//Godot::print(String("Node (") + rigidbody->get_name() + ")FORCED: " + String::num_int64(shape_id) + " [" + String::num_real(force_power) + "]");
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
			PoolIntArray neighbours = _graph->neighbours(node_key);
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
			node["force"] = Vector3::ZERO;
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
				// Single fragment remains, convert to simple rigidbody
				Dictionary node = _graph->_nodes[ar[0]];
				detach_shape( ar[0], node["force"]);
			}
			subgraphs.remove(gidx);

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
				// Single fragment remains, convert to simple rigidbody
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

	RigidBody* new_body = RigidBody::_new();
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
	CollisionShape* colshape = Object::cast_to<CollisionShape>(shape_owner_get_owner(shape_id));
	
	if (colshape == nullptr) {
		return;
	}

	if (bullet_backend) {
		new_body->set_global_transform(colshape->get_global_transform());
		colshape->set_transform(Transform::IDENTITY);
	}

	remove_child(colshape);
	new_body->add_child(colshape);
	colshape->set_owner(new_body);
	
	//force = force.normalized() * Math::clamp(force.length(), 0.0f, 100.0f);
	_graph->remove_node(shape_id);	
	new_body->apply_impulse(Vector3::ZERO, force);
	connectivity_changed = true;
	center_changed = true;
}

void BakedFracture::fracture_subgraph(Array subgraph_nodes) {
	// Engine.time_scale = 0.05
	// Build a new baked fractured with the shapes in subgraph
	BakedFracture* fracture = BakedFracture::_new();

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
	
	if (bullet_backend) {
		Transform t = fracture->get_global_transform();
		t.origin = fracture->to_global(fracture->_graph->centroid());
		fracture->set_global_transform(t);
	}
	
	for (int n = 0; n < subgraph_nodes.size(); n++) {
		CollisionShape* colshape = Object::cast_to<CollisionShape>(shape_owner_get_owner(subgraph_nodes[n]));
		Vector3 colshape_global = colshape->get_global_transform().origin;
		remove_child(colshape);
		fracture->add_child(colshape);
		colshape->set_owner(fracture);
		
		if (bullet_backend) {
			Transform t = colshape->get_global_transform();
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
	Array shape_globals;
	Array shape_owners = get_shape_owners();
	for (int n = 0; n < shape_owners.size(); n++) {
		Array pair;
		pair.append(shape_owners[n]);
		pair.append(Object::cast_to<CollisionShape>(shape_owner_get_owner(shape_owners[n]))->get_global_transform().origin);
		shape_globals.append(pair);
	}

	Transform t = get_global_transform();
	t.origin = to_global(_graph->centroid());
	set_global_transform(t);
	for (int n = 0; n < shape_globals.size(); n++) {
		Array pair = shape_globals[n];
		int shaper_owner = pair[0];
		CollisionShape* colshape = Object::cast_to<CollisionShape>(shape_owner_get_owner(shaper_owner));
		Transform t = colshape->get_global_transform();
		t.origin = pair[1];
		colshape->set_global_transform(t);
		Dictionary node = _graph->_nodes[pair[0]];
		node["position"] = colshape->get_transform().origin;
	}
}

void BakedFracture::mesh_centroid(Mesh* mesh) {

}

void BakedFracture::glue_debug_setup() {
	_debug_line_mat = SpatialMaterial::_new();

	_debug_line_mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
	_debug_line_mat->set_flag(SpatialMaterial::Flags::FLAG_DISABLE_DEPTH_TEST, true);
	_debug_line_mat->set_flag(SpatialMaterial::Flags::FLAG_USE_POINT_SIZE, true);
	_debug_line_mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

	_debug_line_mat->set_albedo(Color(1.0,1.0,1.0,1.0));
	_debug_line_mat->set_line_width(4.0);
	_debug_line_mat->set_cull_mode(SpatialMaterial::CullMode::CULL_DISABLED);
	

	_debug_point_mat = SpatialMaterial::_new();
	_debug_point_mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
	_debug_point_mat->set_flag(SpatialMaterial::Flags::FLAG_DISABLE_DEPTH_TEST, true);
	_debug_point_mat->set_flag(SpatialMaterial::Flags::FLAG_USE_POINT_SIZE, true);
	_debug_point_mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

	_debug_point_mat->set_albedo(Color(1.0,1.0,1.0,1.0));
	_debug_point_mat->set_point_size(10.0);
	_debug_point_mat->set_cull_mode(SpatialMaterial::CullMode::CULL_DISABLED);
	

	_debug_drawer = ImmediateGeometry::_new();
	add_child(_debug_drawer);
	_debug_drawer->clear();
}

void BakedFracture::glue_debug_draw() {
	if (!Engine::get_singleton()->is_editor_hint() && glue_debug_graph && _graph != nullptr) {
		_debug_drawer->clear();
		//_debug_drawer->set_material_override(_debug_line_mat);
		
		// Draw edges
		for (int i = 0; i < _graph->_edges.size(); i++) {
			Dictionary x = _graph->_edges[i];
			_debug_drawer->begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP);
			_debug_drawer->set_color(Color(1.0,1.0,1.0,1.0));
			Dictionary node1 = _graph->_nodes[x["a"]];
			Dictionary node2 = _graph->_nodes[x["b"]];
			_debug_drawer->add_vertex(node1["position"]);
			_debug_drawer->add_vertex(node2["position"]);
			_debug_drawer->end();
		}
		
		_debug_drawer->set_material_override(_debug_point_mat);
		// Draw nodes
		Array keys = _graph->_nodes.keys();
		for (int x = 0; x < keys.size(); x++) {
			Dictionary n = _graph->_nodes[keys[x]];
			_debug_drawer->begin(Mesh::PrimitiveType::PRIMITIVE_POINTS);
			if (n["anchor"]) {
				_debug_drawer->set_color(Color(0,0,1));
			} else {
				if (((Vector3)n["force"]).length() > 0) {
					_debug_drawer->set_color(Color(1,0,0));
				} else {
					_debug_drawer->set_color(Color(0,1,0));
				}
			}
			_debug_drawer->add_vertex(n["position"]);
			_debug_drawer->end();
		}
		
		if (bullet_backend) {
			// draw center of mass
			_debug_drawer->begin(Mesh::PrimitiveType::PRIMITIVE_POINTS);
			_debug_drawer->set_color(Color(0.5, 0.0, 1.0, 1.0));
			_debug_drawer->add_vertex(Vector3());
			_debug_drawer->end();
		}
	}
}

//void glue();

void BakedFracture::set_graph(Ref<Ggraph> graph) {
	//ResourceLoader* loader = ResourceLoader::get_singleton();
	_graph = graph; //loader->load(graph->get_path());
}

Ref<Ggraph> BakedFracture::get_graph() {
	return _graph;
}

void BakedFracture::_register_methods() {
	// Functions
	//register_method("glue_set_debug_graph", &BakedFracture::glue_set_debug_graph);
	register_method("_ready", &BakedFracture::_ready);
	register_method("_process", &BakedFracture::_process);
	register_method("update", &BakedFracture::update);
	register_method("set_anchor_enabled", &BakedFracture::set_anchor_enabled);
	register_method("_integrate_forces", &BakedFracture::_integrate_forces);
	register_method("propagate", &BakedFracture::propagate);
	register_method("detach_shape", &BakedFracture::detach_shape);
	register_method("fracture_subgraph", &BakedFracture::fracture_subgraph);
	register_method("recalculate_center_of_mass", &BakedFracture::recalculate_center_of_mass);
	register_method("mesh_centroid", &BakedFracture::mesh_centroid);
	register_method("glue_debug_setup", &BakedFracture::glue_debug_setup);
	register_method("glue_debug_draw", &BakedFracture::glue_debug_draw);

	// Properties
	register_property("glue_debug_graph", &BakedFracture::set_debug_graph, &BakedFracture::get_debug_graph, false); 
	register_property("glue_debug_graph", &BakedFracture::glue_debug_graph, false);
	register_property("glue_margin", &BakedFracture::glue_margin, 0.005f);
	register_property("glue_strength", &BakedFracture::glue_strength, 500.0f);
	register_property("force_threshold", &BakedFracture::force_threshold, 1.0f);
	register_property("anchor_enabled", &BakedFracture::anchor_enabled, false);
	register_property("anchor_static", &BakedFracture::anchor_static, false);
	register_property<BakedFracture, Ref<GDScript>>("piece_script", &BakedFracture::piece_script, nullptr);
	register_property<BakedFracture, String>("group", &BakedFracture::group, "");
	
	register_property<BakedFracture, Node*>("piece_parent", &BakedFracture::piece_parent, nullptr);
	//register_property<BakedFracture, Ref<Ggraph>>("_graph", &BakedFracture::_graph, Ref<Ggraph>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE);
    register_property<BakedFracture, Ref<Ggraph>>("_graph", &BakedFracture::set_graph, &BakedFracture::get_graph, Ref<Ggraph>(),  GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE);
	register_property("connectivity_changed", &BakedFracture::connectivity_changed, false);

	// Signals
	register_signal<BakedFracture>((char*)"fractured");
}