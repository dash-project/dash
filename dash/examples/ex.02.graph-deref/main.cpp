#include <libdash.h>
#include <iostream>
#include <ctime>
#include <math.h>
// #include "rmatrandom.h"

struct vprop {
  int comp;
};

struct eprop {
  int comp;
};

typedef dash::Graph<dash::DirectedGraph, vprop, eprop>       graph_t;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  int n_unit_edges = 1;
  int n_vertices_full = 448;
  int n_edges_full = n_vertices_full * n_unit_edges;
  // not exactly n_vertices_full are generated due to rounding
  int n_vertices_start = n_vertices_full / dash::size();
  int n_size_rounds = 5;
  int n_rounds = 1;
  for(int rounds = 0; rounds < n_size_rounds; ++rounds) {
    for(int i = 0; i < n_rounds; ++i) {
      int scale = rounds + 1;
      int n_vertices = n_vertices_start * scale;
      graph_t g(n_vertices, n_unit_edges);

      for(int j = 0; j < n_vertices; ++j) {
        g.add_vertex();
      }

      g.commit();

      int n_edges = n_vertices * n_unit_edges;
      auto src = g.vertices().lbegin();
      auto trg = src + 1;
      auto end = g.vertices().lend();
      for(int j = 0; j < n_edges; ++j) {
        g.add_edge(src, trg);
        ++src;
        ++trg;
        if(trg == end) {
          src = g.vertices().lbegin();
          trg = src + 1;
        }
      }

      g.commit();

      std::clock_t g_begin_time = clock();
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        v.attributes();
      }
      std::clock_t g_end_time = clock();
      double time = double(g_end_time - g_begin_time) / CLOCKS_PER_SEC;

      double all_time;
      dart_reduce(&time, &all_time, 1, DART_TYPE_DOUBLE, DART_OP_SUM, 0, 
          g.team().dart_id());

      if(dash::myid() == 0) {
        std::cout << "[round " << i << "] " << n_vertices_full * scale << 
          " vertices per node dereferenced (local): " << all_time << 
          std::endl;
      }

      if(dash::myid() == 0) {
        std::clock_t g_begin_time = clock();
        for(auto it = g.vertices().begin(); it != g.vertices().end(); ++it) {
         auto v = g[it];
         v.attributes();
        }
        std::clock_t g_end_time = clock();
        std::cout << "[round " << i << "] " << n_vertices_full * scale << 
          " vertices per node dereferenced (global): " << 
          double(g_end_time - g_begin_time) / CLOCKS_PER_SEC << std::endl;
      }

      g_begin_time = clock();
      for(auto it = g.out_edges().lbegin(); it != g.out_edges().lend(); ++it) {
        auto e = g[it];
        e.attributes();
      }
      g_end_time = clock();
      time = double(g_end_time - g_begin_time) / CLOCKS_PER_SEC;

      dart_reduce(&time, &all_time, 1, DART_TYPE_DOUBLE, DART_OP_SUM, 0, 
          g.team().dart_id());

      if(dash::myid() == 0) {
        std::cout << "[round " << i << "] " << n_edges_full * scale << 
          " edges per node dereferenced (local): " << all_time << std::endl;
      }

      if(dash::myid() == 0) {
        std::clock_t g_begin_time = clock();
        for(auto it = g.out_edges().begin(); it != g.out_edges().end(); ++it) {
          auto e = g[it];
          e.attributes();
        }
        std::clock_t g_end_time = clock();
        std::cout << "[round " << i << "] " << n_edges_full * scale << 
          " edges per node dereferenced (global): " << 
          double(g_end_time - g_begin_time) / CLOCKS_PER_SEC << std::endl;
      }
      dash::barrier();
    }
    if(dash::myid() == 0) std::cout << "-----------------" << std::endl;
  }

  dash::finalize();
  return 0;
} 


