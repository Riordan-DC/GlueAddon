/* baked_fracture.h */

#ifndef BAKEDFRACTURE_H
#define BAKEDFRACTURE_H

#include "ggraph.h"

#include <Godot.hpp>
#include <RigidBody.hpp>
#include <ImmediateGeometry.hpp>
#include <Texture.hpp>
#include <Mesh.hpp>
#include <SpatialMaterial.hpp>
#include <PhysicsDirectBodyState.hpp>
#include <Vector3.hpp>
#include <Engine.hpp>
#include <CollisionShape.hpp>
#include <Math.hpp>
#include <PhysicsServer.hpp>
#include <ResourceLoader.hpp>
#include <String.hpp>
#include <OS.hpp>
#include <GDScript.hpp>

using namespace godot;

class BakedFracture : public RigidBody {
    GODOT_CLASS(BakedFracture, RigidBody);
public:
    BakedFracture();
    ~BakedFracture();
    void _init();

    void set_debug_graph(bool value);
    bool get_debug_graph();

    void _ready();
    void _process(float p_delta);
    void update();
    void _physics_process(float p_delta);
    void set_anchor_enabled(bool value);
    void _integrate_forces(PhysicsDirectBodyState* state);
    void propagate(bool force_update = false);
    void detach_shape(int shape_id, Vector3 force);
    void fracture_subgraph(Array subgraph_nodes);
    void recalculate_center_of_mass();
    void mesh_centroid(Mesh* mesh);
    void glue_debug_setup();
    void glue_debug_draw();

    void set_graph(Ref<Ggraph> graph);
    Ref<Ggraph> get_graph();
    //void glue();

    static void _register_methods();

    ImmediateGeometry* _debug_drawer;
    SpatialMaterial* _debug_line_mat;
    SpatialMaterial* _debug_point_mat;
    Ref<GDScript> piece_script;

    bool bullet_backend = true; //ProjectSettings.get("physics/3d/physics_engine") in ["DEFAULT", "Bullet"]
    bool center_changed = false;
    bool connectivity_changed = false;

    Vector3 velocity_inital;

    // Exports
    bool glue_debug_graph = false; // setget glue_set_debug_graph
    // Glue margin
    float glue_margin = 0.005f;
    // Strength of each bond
    float glue_strength = 1000.0f;
    // Force threshold
    float force_threshold = 1.0f;
    // Glue anchor nodes stop its owner from moving
    bool anchor_enabled = false; // setget set_anchor_enabled
    // Anchors will not recieve force
    bool anchor_static = false;
    // start locked to ground
    Ref<Ggraph> _graph;
    // Piece parent
    Node* piece_parent = nullptr;
    // Piece Group
    String group = "";

    // UNUSED
    //bool flex = false;
    //float flex_strength = 0.5f;
};



#endif 