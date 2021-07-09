#include <libdash.h>
#include <iostream>
#include <ctime>
#include <cstdlib>

#include "rmatrandom.h"

struct vprop {
  dash::default_index_t comp;
  dash::global_unit_t   unit;
};

typedef dash::Graph<dash::UndirectedGraph, vprop>       graph_t;

int main(int argc, char* argv[]) {
  if(argc == 3) {
    dash::init(&argc, &argv);
    int n_vertices = 1000 * atoi(argv[1]);
    int n_edges = 4000 * atoi(argv[2]);
    auto & team = dash::Team::All();
    dash::LogarithmicVertexMapper<graph_t> mapper(n_vertices, team.size());
    for(int i = 0; i < 5; ++i) {
      RMATRandomGenerator<graph_t> begin(n_vertices, n_edges, team.size(), 
          team.myid(), mapper, 0.25, 0.25, 0.25, 0.25);
      RMATRandomGenerator<graph_t> end(0, 0, team.size(), team.myid(), mapper, 
          0, 0, 0, 0);

      std::clock_t g_begin_time = clock();
      graph_t g(begin, end, n_vertices, team, mapper);
      std::clock_t g_end_time = clock();
      if(dash::myid() == 0) {
	      std::cout << "[round " << i + 1 << "] construction: " << 
          double(g_end_time - g_begin_time) / CLOCKS_PER_SEC << std::endl;
      }

      dash::barrier();
      std::clock_t begin_time = clock();

      //dash::util::TraceStore::on();
      //dash::util::TraceStore::clear();

      dash::connected_components(g);

      //dash::barrier();
      //dash::util::TraceStore::off();
      //dash::util::TraceStore::write(std::cout);

      std::clock_t end_time = clock();
      if(dash::myid() == 0) {
	      std::cout << "[round " << i + 1 << "] algorithm: " << 
          double(end_time - begin_time) / CLOCKS_PER_SEC << std::endl;
      }
    }
    dash::finalize();
  }
  return 0;
}

