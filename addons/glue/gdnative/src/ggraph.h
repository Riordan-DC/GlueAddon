/* ggraph.h */

#ifndef GGRAPH_H
#define GGRAPH_H

#include <assert.h>     /* assert */

#include <Godot.hpp>
#include <Resource.hpp>
#include <String.hpp>
#include <OS.hpp>
#include <BitMap.hpp>
#include <Color.hpp>

using namespace godot;

#define SET_BIT(A,n) (A[(n)/32] |= (1 << ((k) % 32)))
#define CLEAR_BIT(A,n) (A[(n)/32] &= ~(1 << ((k) % 32)))
#define BIT_VALUE(A,n) (A[(n)/32] & (1 << ((k) % 32)))

class Ggraph : public Resource {
    GODOT_CLASS(Ggraph, Resource);
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
    PoolIntArray neighbours(int x);
    Vector3 centroid();
    Array subgraphs();
    Array nodes_edges(Array nodes);
    Ggraph* remove_subgraph(Array nodes);

    Dictionary get_nodes();
    void set_nodes(Dictionary nodes);
    Array get_edges();
    void set_edges(Array edges);

    static void _register_methods();
    Dictionary _nodes;
    Array _edges;

    bool using_edge_matrix;
    Dictionary get_edge_matrix();
    void set_edge_matrix(Ref<BitMap> p_edge_matrix);
    void setup_edge_matrix();
    bool test_edge_matrix();
    Ref<BitMap> edge_matrix;
};



#endif 