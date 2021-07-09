#include <libdash.h>
#include <iostream>
#include <ctime>
#include <math.h>

#include "rmatrandom.h"

struct vprop {
  int comp;
};

struct eprop {
  int comp;
};

typedef dash::Graph<dash::DirectedGraph, vprop, eprop>       graph_t;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  int n_vertices_start = 1000;
  if(dash::myid() != 0) {
    n_vertices_start = 0;
  }
  int n_unit_edges = 1;
  int n_rounds = 1;
  int n_size_rounds = 4;
  for(int rounds = 3; rounds < n_size_rounds; ++rounds) {
    for(int i = 0; i < n_rounds; ++i) {
      int n_vertices = n_vertices_start * pow(10, rounds);
      graph_t g(n_vertices, n_unit_edges);

      std::clock_t g_begin_time = clock();
      for(int j = 0; j < n_vertices; ++j) {
        g.add_vertex();
      }
      std::clock_t g_end_time = clock();
      if(dash::myid() == 0) {
        std::cout << "[round " << i << "] " << n_vertices << 
          " vertices added: " <<  double(g_end_time - g_begin_time) 
          / CLOCKS_PER_SEC << std::endl;
      }

      g.commit();
/*
      int n_edges = n_vertices * n_unit_edges;
      if(dash::myid() != 0) {
        n_edges = 0;
      }
      auto src = g.vertices().lbegin();
      auto trg = src + 1;
      auto end = g.vertices().lend();
      g_begin_time = clock();
      for(int j = 0; j < n_edges; ++j) {
        g.add_edge(src, trg);
        ++src;
        ++trg;
        if(trg == end) {
          src = g.vertices().lbegin();
          trg = src + 1;
        }
      }
      g_end_time = clock();
      if(dash::myid() == 0) {
        std::cout << "[round " << i << "] " << n_edges << " edges added: " << 
          double(g_end_time - g_begin_time) / CLOCKS_PER_SEC << std::endl;
      }
*/
    }
    dash::barrier();
    if(dash::myid() == 0) std::cout << "----------------------" << std::endl;
  }

  dash::finalize();
  return 0;
} 

