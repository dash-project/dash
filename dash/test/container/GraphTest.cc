
#include "GraphTest.h"
#include <dash/GlobSharedRef.h>
#include <dash/Graph.h>

struct vertex_prop {
  int id;
};

struct edge_prop {
  int id;
};

TEST_F(GraphTest, GlobalIteration)
{
  typedef int                        value_t;
  typedef dash::default_index_t      vertex_index_t;
  typedef dash::default_index_t      edge_index_t;
  typedef dash::Graph<
    dash::DirectedGraph, 
    void, 
    vertex_prop,
    edge_prop,
    vertex_index_t,
    edge_index_t,
    std::vector,
    std::vector>                      graph_t;
  typedef graph_t::vertex_index_type  vertex_index_type;
  typedef graph_t::vertex_type        vertex_type;
  typedef graph_t::edge_type        edge_type;

  auto nunits    = dash::size();
  auto myid      = dash::myid();

  auto ninsert_vertices_per_unit = 3;
  auto ninsert_edges_per_vertex = 3;
  ASSERT_LE(ninsert_edges_per_vertex, ninsert_vertices_per_unit);
  auto ninsert_edges_per_unit = ninsert_vertices_per_unit * 
    ninsert_edges_per_vertex;
  
  auto vertex_cap = 1 * nunits;
  auto edge_per_vertex_cap = 1;

  graph_t graph(vertex_cap, edge_per_vertex_cap);

  EXPECT_EQ_U(0, graph.num_vertices());
  EXPECT_EQ_U(0, graph.num_edges());
  EXPECT_EQ_U(-1, graph.max_vertex_index());
  EXPECT_EQ_U(-1, graph.max_edge_index());
  EXPECT_TRUE_U(graph.empty());

  dash::barrier();
  DASH_LOG_DEBUG("GraphTest.GlobalIteration", "graph initialized");
  int ei = 0;
  for(int i = 0; i < ninsert_vertices_per_unit; ++i) {
    vertex_prop vprop { nunits  * i + myid };
    auto source = graph.add_vertex(vprop);
    //TODO: test vertex indeices for already existing vertices
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "vertex inserted");
    auto target_unit = (myid + 1) % nunits;
    for(int j = 0; j < ninsert_edges_per_vertex; ++j) {
      vertex_index_type target(dash::team_unit_t(target_unit), j);
      edge_prop eprop { nunits * ei + myid };
      graph.add_edge(source, target, eprop);
      ++ei;
    }
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "edge inserted");
  }

  graph.barrier();
  DASH_LOG_DEBUG("GraphTest.GlobalIteration", "elements committed");

  if(myid == 0) {
    int i = 0;
    int unit_id = 0;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "begin vertex iteration");
    for(auto it = graph.vertices.begin(); it != graph.vertices.end(); ++it) {
      int id = nunits * i + unit_id;
      vertex_type v = *it;
      DASH_LOG_DEBUG("GraphTest.GlobalIteration", "vertex",
          "it", it,
          "value", v.properties.id);
      EXPECT_EQ_U(id, v.properties.id);
      ++i;
      if(i == ninsert_vertices_per_unit) {
        i = 0;
        ++unit_id;
      }
    }

    i = 0;
    unit_id = 0;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "begin out-edge iteration");
    for(auto it = graph.out_edges.begin(); it != graph.out_edges.end(); ++it) {
      int id = nunits * i + unit_id;
      edge_type e = *it;
      DASH_LOG_DEBUG("GraphTest.GlobalIteration", "out-edge",
          "it", it,
          "value", e.properties.id);
      EXPECT_EQ_U(e.properties.id, id);
      ++i;
      if(i == ninsert_edges_per_unit) {
        i = 0;
        ++unit_id;
      }
    }

    int j = 0;
    int k = 0;
    int l = 0;
    int unit_id_prev = nunits - 1;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "begin in-edge iteration");
    for(auto it = graph.in_edges.begin(); it != graph.in_edges.end(); ++it) {
      int id = nunits * (j * ninsert_edges_per_vertex) + (nunits * k) + 
        unit_id_prev;
      edge_type e = *it;
      DASH_LOG_DEBUG("GraphTest.GlobalIteration", "in-edge",
          "it", it,
          "value", e.properties.id);
      EXPECT_EQ_U(e.properties.id, id);
      ++j;
      if(j == ninsert_edges_per_vertex) {
        j = 0;
        k += 1;
      }
      ++l;
      if(l == ninsert_edges_per_unit) {
        l = 0;
        unit_id_prev = (unit_id_prev + 1) % nunits;
        k = 0;
      }
    }

    i = 0;
    j = 0;
    k = 0;
    l = 0;
    int m = 0;
    unit_id = 0;
    unit_id_prev = nunits - 1;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", "begin edge iteration");
    for(auto it = graph.edges.begin(); it != graph.edges.end(); ++it) {
      edge_type e = *it;
      int id;
      if(m < ninsert_edges_per_vertex) {
        id = nunits * i + unit_id;
        ++i;
        if(i == ninsert_edges_per_unit) {
          i = 0;
          ++unit_id;
        }
      } else {
        id = nunits * (j * ninsert_edges_per_vertex) + (nunits * k) + 
          unit_id_prev;
        ++j;
        if(j == ninsert_edges_per_vertex) {
          j = 0;
          k += 1;
        }
        ++l;
        if(l == ninsert_edges_per_unit) {
          l = 0;
          unit_id_prev = (unit_id_prev + 1) % nunits;
          k = 0;
        }
      }
      ++m;
      if(m == 2 * ninsert_edges_per_vertex) {
        m = 0;
      }
      DASH_LOG_DEBUG("GraphTest.GlobalIteration", "edge",
          "it", it,
          "value", e.properties.id);
      EXPECT_EQ_U(e.properties.id, id);
    }
    
    i = 0;
    unit_id = 0;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", 
        "begin adjacency out-edge iteration");
    for(auto it = graph.vertices.begin(); it != graph.vertices.end(); ++it) {
      vertex_type v = *it;
      for(auto ait = graph.out_edges.vbegin(v); 
          ait != graph.out_edges.vend(v); ++ait) {
        int id = nunits * i + unit_id;
        edge_type e = *ait;
        DASH_LOG_DEBUG("GraphTest.GlobalIteration", "out-edge",
            "it", ait,
            "value", e.properties.id);
        EXPECT_EQ_U(e.properties.id, id);
        ++i;
        if(i == ninsert_edges_per_unit) {
          i = 0;
          ++unit_id;
        }
      }
    }

    j = 0;
    k = 0;
    l = 0;
    unit_id_prev = nunits - 1;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", 
        "begin adjacency in-edge iteration");
    for(auto it = graph.vertices.begin(); it != graph.vertices.end(); ++it) {
      vertex_type v = *it;
      for(auto ait = graph.in_edges.vbegin(v); 
          ait != graph.in_edges.vend(v); ++ait) {
        int id = nunits * (j * ninsert_edges_per_vertex) + (nunits * k) + 
          unit_id_prev;
        edge_type e = *ait;
        DASH_LOG_DEBUG("GraphTest.GlobalIteration", "in-edge",
            "it", ait,
            "value", e.properties.id);
        EXPECT_EQ_U(e.properties.id, id);
        ++j;
        if(j == ninsert_edges_per_vertex) {
          j = 0;
          k += 1;
        }
        ++l;
        if(l == ninsert_edges_per_unit) {
          l = 0;
          unit_id_prev = (unit_id_prev + 1) % nunits;
          k = 0;
        }
      }
    }

    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    unit_id = 0;
    unit_id_prev = nunits - 1;
    DASH_LOG_DEBUG("GraphTest.GlobalIteration", 
        "begin adjacency edge iteration");
    for(auto it = graph.vertices.begin(); it != graph.vertices.end(); ++it) {
      vertex_type v = *it;
      for(auto ait = graph.edges.vbegin(v); 
          ait != graph.edges.vend(v); ++ait) {
        edge_type e = *ait;
        int id;
        if(m < ninsert_edges_per_vertex) {
          id = nunits * i + unit_id;
          ++i;
          if(i == ninsert_edges_per_unit) {
            i = 0;
            ++unit_id;
          }
        } else {
          id = nunits * (j * ninsert_edges_per_vertex) + (nunits * k) + 
            unit_id_prev;
          ++j;
          if(j == ninsert_edges_per_vertex) {
            j = 0;
            k += 1;
          }
          ++l;
          if(l == ninsert_edges_per_unit) {
            l = 0;
            unit_id_prev = (unit_id_prev + 1) % nunits;
            k = 0;
          }
        }
        ++m;
        if(m == 2 * ninsert_edges_per_vertex) {
          m = 0;
        }
        DASH_LOG_DEBUG("GraphTest.GlobalIteration", "edge",
            "it", ait,
            "value", e.properties.id);
      }
    }
  }
  dash::barrier();
}

