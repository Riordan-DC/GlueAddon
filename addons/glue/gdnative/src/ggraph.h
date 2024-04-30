/* ggraph.h */

#ifndef GGRAPH_H
#define GGRAPH_H

#include <assert.h>     /* assert */

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/bit_map.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

#define SET_BIT(A,n) (A[(n)/32] |= (1 << ((k) % 32)))
#define CLEAR_BIT(A,n) (A[(n)/32] &= ~(1 << ((k) % 32)))
#define BIT_VALUE(A,n) (A[(n)/32] & (1 << ((k) % 32)))

class Ggraph : public Resource {
    GDCLASS(Ggraph, Resource);
    /*
    struct GVertex {
        Vector3 position;
        Vector3 force;
        float bond;
        bool anchor;
        int group;
    };*/
public:
    Ggraph();
    ~Ggraph();
    void _init();
    void add_node(int x, Vector3 position);
    void remove_node(int x);
    void add_edge(int x, int y);
    void remove_edge(int x, int y);
    bool adjacent(int x, int y);
    PackedInt32Array neighbours(int x);
    Vector3 centroid();
    Array subgraphs();
    Array nodes_edges(Array nodes);
    Ggraph* remove_subgraph(Array nodes);

    Dictionary get_nodes();
    void set_nodes(Dictionary nodes);
    Array get_edges();
    void set_edges(Array edges);

    static void _bind_methods();
    Dictionary _nodes;
    Array _edges;

    bool using_edge_matrix;
    Dictionary get_edge_matrix();
    void set_edge_matrix(BitMap* p_edge_matrix);
    void setup_edge_matrix();
    bool test_edge_matrix();
    BitMap* edge_matrix;
};



#endif 