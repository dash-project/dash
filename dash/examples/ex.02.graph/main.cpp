#include <iostream>
#include <libdash.h>

struct vprop {
  int id;
  int level;
};

struct eprop {
  int id;
};

typedef dash::Graph<dash::UndirectedGraph, void, vprop, eprop> graph_t;
typedef graph_t::vertex_index_type vertex_index_t;
typedef graph_t::vertex_type vertex_t;
typedef graph_t::edge_type edge_t;

void create_graph(graph_t &g) {
  // add 4 v to every unit
  for (int i = (dash::myid() * 4) + 1; i <= (dash::myid() * 4) + 4; ++i) {
    vprop prop{i, -1};
    g.add_vertex(prop);
  }

  std::vector<vertex_index_t> v;
  v.emplace_back(dash::team_unit_t(0), 0);
  v.emplace_back(dash::team_unit_t(0), 1);
  v.emplace_back(dash::team_unit_t(0), 2);
  v.emplace_back(dash::team_unit_t(0), 3);
  v.emplace_back(dash::team_unit_t(1), 0);
  v.emplace_back(dash::team_unit_t(1), 1);
  v.emplace_back(dash::team_unit_t(1), 2);
  v.emplace_back(dash::team_unit_t(1), 3);

  if (dash::myid() == 0) {
    g.add_edge(v[0], v[1]);
    g.add_edge(v[0], v[4]);
    g.add_edge(v[1], v[5]);
    g.add_edge(v[1], v[6]);
    g.add_edge(v[2], v[6]);
    g.add_edge(v[3], v[7]);
  }
  if (dash::myid() == 1) {
    g.add_edge(v[4], v[5]);
    g.add_edge(v[5], v[6]);
    g.add_edge(v[6], v[7]);
  }

  g.barrier();
}

void breadth_first_search(graph_t &g, vertex_index_t source) {
  dash::Team &team = g.team();

  std::vector<vertex_index_t> frontier;
  std::vector<std::list<vertex_index_t>> neighbours(team.size());

  if (source.unit == team.myid()) {
    frontier.push_back(source);
  }

  int level = 0;
  while (true) {
    for (auto f_it = frontier.begin(); f_it != frontier.end(); ++f_it) {
      vertex_index_t v_index = *f_it;
      vertex_t &v = g.vertices[v_index];
      if (v.properties.level == -1) {
        for (auto e_it = g.out_edges.vbegin(v); e_it != g.out_edges.vend(v);
             ++e_it) {
          edge_t e = *e_it;
          auto target = e.target();
          neighbours[target.unit.id].push_back(target);
        }
        v.properties.level = level;
      }
    }
    std::cout << neighbours[0].size() << " " << neighbours[1].size()
              << std::endl;

    std::vector<vertex_index_t> neighbour_data;
    std::vector<std::size_t> neighbour_count(team.size());
    std::vector<std::size_t> neighbour_displs(team.size());
    for (int i = 0; i < neighbours.size(); ++i) {
      for (auto &neighbour : neighbours[i]) {
        neighbour_data.push_back(neighbour);
        neighbour_count[i] += sizeof(vertex_index_t);
      }
      for (int j = i + 1; j < neighbour_displs.size(); ++j) {
        neighbour_displs[j] += neighbour_count[i];
      }
      neighbours[i].clear();
    }
    std::vector<std::size_t> neighbour_recv_count(team.size());
    dart_alltoall(neighbour_count.data(), neighbour_recv_count.data(),
                  sizeof(std::size_t), DART_TYPE_BYTE, team.dart_id());
    int total = 0;
    std::vector<std::size_t> neighbour_recv_displs(team.size());
    for (int i = 0; i < neighbour_recv_count.size(); ++i) {
      total += neighbour_recv_count[i];
      for (int j = i + 1; j < neighbour_recv_displs.size(); ++j) {
        neighbour_recv_displs[j] += neighbour_recv_count[i];
      }
    }
    int global_count = 0;
    dart_allreduce(&total, &global_count, 1, DART_TYPE_INT,
                   DART_OP_SUM, team.dart_id());
    // frontier is empty on all nodes -> terminate algorithm
    if (global_count == 0) {
      break;
    }
    std::vector<vertex_index_t> new_frontier(total / sizeof(vertex_index_t));
    dart_alltoallv(neighbour_data.data(), neighbour_count.data(),
                   neighbour_displs.data(), DART_TYPE_BYTE, new_frontier.data(),
                   neighbour_recv_count.data(), neighbour_recv_displs.data(),
                   team.dart_id());

    frontier = new_frontier;

    ++level;
  }
}

int main(int argc, char *argv[]) {
  dash::init(&argc, &argv);
  graph_t g(8);

  create_graph(g);
  breadth_first_search(g, vertex_index_t(dash::team_unit_t(0), 0));
  if (dash::myid() == 1) {
    std::cout << g.num_vertices() << std::endl;
    for (auto it = g.vertices.begin(); it != g.vertices.end(); ++it) {
      vertex_t v = *it;
      std::cout << "vertex " << v.properties.id << ": level "
                << v.properties.level << std::endl;
    }
    /*
    for(auto it = g.out_edges.begin(); it != g.out_edges.end(); ++it) {
      edge_t e = *it;
      std::cout << "edge: " << e.source().offset << " -> " << e.target().offset
    << std::endl;
    }
    */
  }
  dash::barrier();
}
