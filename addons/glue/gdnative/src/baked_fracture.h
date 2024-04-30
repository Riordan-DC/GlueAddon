/* baked_fracture.h */

#ifndef BAKEDFRACTURE_H
#define BAKEDFRACTURE_H

#include "ggraph.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/physics_direct_body_state3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

class BakedFracture : public RigidBody3D {
    GDCLASS(BakedFracture, RigidBody3D);

protected:
    static void _bind_methods();

private:
    ImmediateMesh* _debug_drawer;
    StandardMaterial3D* _debug_line_mat;
    StandardMaterial3D* _debug_point_mat;

    bool bullet_backend = true; //ProjectSettings.get("physics/3d/physics_engine") in ["DEFAULT", "Bullet"]
    bool jolt_backend = false;
    bool center_changed = false;

    Vector3 velocity_inital;

public:
    BakedFracture();
    ~BakedFracture();
    void _init();

    void _ready();
    void _process(double p_delta);
    void update();
    void _integrate_forces(PhysicsDirectBodyState3D* state);
    void propagate(bool force_update = false);
    void detach_shape(int shape_id, Vector3 force);
    void fracture_subgraph(Array subgraph_nodes);
    void recalculate_center_of_mass();
    void glue_debug_setup();
    void glue_debug_draw();

    // Exports
    void set_glue_debug_graph(bool value) { glue_debug_graph = value;}
    bool get_glue_debug_graph(void) { return glue_debug_graph; }
    bool glue_debug_graph = false; // setget glue_set_debug_graph
    // Glue margin
    void set_glue_margin(float margin) { glue_margin = margin; }
    float get_glue_margin(void) { return glue_margin; }
    float glue_margin = 0.005f;
    // Strength of each bond
    void set_glue_strength(float strength) { glue_strength = strength; }
    float get_glue_strength(void) { return glue_strength; }
    float glue_strength = 1000.0f;
    // Force threshold
    void set_force_threshold(float threshold) { force_threshold = threshold; }
    float get_force_threshold(void) { return force_threshold; }
    float force_threshold = 1.0f;
    // Glue anchor nodes stop its owner from moving
    void set_anchor_enabled(bool value);
    bool get_anchor_enabled(void) { return anchor_enabled; }
    bool anchor_enabled = false; // setget set_anchor_enabled
    // Anchors will not recieve force
    void set_anchor_static(bool value) { anchor_static = value; }
    bool get_anchor_static(void) { return anchor_static; }
    bool anchor_static = false;
    // start locked to ground
    void set_graph(Ref<Ggraph> graph);
    Ref<Ggraph> get_graph();
    Ref<Ggraph> _graph;
    // Piece parent
    void set_piece_parent(Node* node) { piece_parent = node; }
    Node* get_piece_parent(void) { return piece_parent; }
    Node* piece_parent = nullptr;
    // Piece Group
    void set_group(String value) { group = value; }
    String get_group(void) { return group; }
    String group = "";
    // Piece script
    void set_piece_script(Ref<GDScript> script) { piece_script = script; }
    Ref<GDScript> get_piece_script(void) { return piece_script; }
    Ref<GDScript> piece_script;

    void set_connectivity_changed(bool value) { connectivity_changed = value; }
    bool get_connectivity_changed(void) { return connectivity_changed; }
    bool connectivity_changed = false;


    // UNUSED
    //bool flex = false;
    //float flex_strength = 0.5f;
};



#endif 