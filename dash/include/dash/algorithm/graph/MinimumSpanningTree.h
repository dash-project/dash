#ifndef DASH__ALGORITHM____GRAPH__MINIMUM_SPANNING_TREE_H__
#define DASH__ALGORITHM____GRAPH__MINIMUM_SPANNING_TREE_H__

#include <random>
#include <dash/Graph.h>
#include <dash/util/Trace.h>

namespace dash {

typedef std::vector<std::vector<std::pair<int, int>>>     pair_t;
typedef std::vector<std::vector<int>>                     matrix_t;

// (supervertex, new component, weight, owner, local offset)
template<typename T>
using tuple_t = std::tuple<int, T, int, int, int>;
template<typename T>
using matrix_min_pairs_t = std::vector<std::vector<tuple_t<T>>>;

template<typename T> 
using matrix_pair_t = std::vector<std::vector<std::pair<int, T>>>;

namespace internal {

template<typename GraphType>
auto mst_get_data(
    matrix_t & indices, 
    matrix_t & permutations, 
    GraphType & graph,
    dash::util::Trace & trace
) -> std::vector<typename GraphType::vertex_properties_type> {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  trace.enter_state("send indices");
  // exchange indices to get
  std::vector<std::size_t> sizes_send(indices.size());
  std::vector<std::size_t> displs_send(indices.size());
  std::vector<std::size_t> sizes_recv_data(indices.size());
  std::vector<std::size_t> displs_recv_data(indices.size());
  std::vector<int> indices_send;
  std::size_t total_send = 0;
  for(int i = 0; i < indices.size(); ++i) {
    sizes_send[i] = indices[i].size();
    displs_send[i] = total_send;
    sizes_recv_data[i] = indices[i].size() * sizeof(vprop_t);
    displs_recv_data[i] = total_send * sizeof(vprop_t);
    total_send += indices[i].size();
  }
  indices_send.reserve(total_send);
  for(auto & index_set : indices) {
    indices_send.insert(indices_send.end(), index_set.begin(), 
        index_set.end());
  }
  std::vector<std::size_t> sizes_recv(indices.size());
  std::vector<std::size_t> displs_recv(indices.size());
  std::vector<std::size_t> sizes_send_data(indices.size());
  std::vector<std::size_t> displs_send_data(indices.size());
  dart_alltoall(sizes_send.data(), sizes_recv.data(), sizeof(std::size_t), 
      DART_TYPE_BYTE, graph.team().dart_id());
  std::size_t total_recv = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    sizes_send_data[i] = sizes_recv[i] * sizeof(vprop_t);
    displs_send_data[i] = total_recv * sizeof(vprop_t);
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }
  std::vector<int> indices_recv(total_recv);
  dart_alltoallv(indices_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_INT, indices_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());

  std::vector<vprop_t> data_send;
  data_send.reserve(total_recv);
  std::vector<vprop_t> data_recv(total_send);

  trace.exit_state("send indices");
  // exchange data
  trace.enter_state("get components");
  for(auto & index : indices_recv) {
    data_send.push_back(graph.vertex_attributes(index));
  }
  trace.exit_state("get components");
  trace.enter_state("send components");
  dart_alltoallv(data_send.data(), sizes_send_data.data(), 
      displs_send_data.data(), DART_TYPE_BYTE, data_recv.data(), 
      sizes_recv_data.data(), displs_recv_data.data(), 
      graph.team().dart_id());
  trace.exit_state("send components");

  trace.enter_state("restore order");
  // restore original order
  // TODO: use more sophisticated ordering mechanism
  std::vector<vprop_t> output(data_recv.size());
  int index = 0;
  for(int i = 0; i < permutations.size(); ++i) {
    for(int j = 0; j < permutations[i].size(); ++j) {
      output[permutations[i][j]] = data_recv[index];
      ++index;
    }
  }
  trace.exit_state("restore order");
  return output;
}

template<typename GraphType>
auto mst_get_components(
    matrix_t & indices, 
    int start,
    GraphType & graph,
    dash::util::Trace & trace
) -> std::unordered_map<int, typename GraphType::vertex_properties_type> {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  trace.enter_state("send indices");
  // exchange indices to get
  std::vector<std::size_t> sizes_send(indices.size());
  std::vector<std::size_t> displs_send(indices.size());
  std::vector<std::size_t> sizes_recv_data(indices.size());
  std::vector<std::size_t> displs_recv_data(indices.size());
  std::vector<int> indices_send;
  int total_send = 0;
  for(int i = 0; i < indices.size(); ++i) {
    sizes_send[i] = indices[i].size();
    displs_send[i] = total_send;
    sizes_recv_data[i] = indices[i].size() * sizeof(vprop_t);
    displs_recv_data[i] = total_send * sizeof(vprop_t);
    total_send += indices[i].size();
  }
  indices_send.reserve(total_send);
  for(auto & index_set : indices) {
    indices_send.insert(indices_send.end(), index_set.begin(), 
        index_set.end());
  }
  std::vector<std::size_t> sizes_recv(indices.size());
  std::vector<std::size_t> displs_recv(indices.size());
  std::vector<std::size_t> sizes_send_data(indices.size());
  std::vector<std::size_t> displs_send_data(indices.size());
  dart_alltoall(sizes_send.data(), sizes_recv.data(), sizeof(std::size_t), 
      DART_TYPE_BYTE, graph.team().dart_id());
  int total_recv = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    sizes_send_data[i] = sizes_recv[i] * sizeof(vprop_t);
    displs_send_data[i] = total_recv * sizeof(vprop_t);
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }
  std::vector<int> indices_recv(total_recv);
    dart_alltoallv(indices_send.data(), sizes_send.data(), displs_send.data(),
        DART_TYPE_INT, indices_recv.data(), sizes_recv.data(), 
        displs_recv.data(), graph.team().dart_id());

  std::vector<vprop_t> data_send;
  data_send.reserve(total_recv);
  std::vector<vprop_t> data_recv(total_send);

  trace.exit_state("send indices");
  // exchange data
  trace.enter_state("get components");
  for(auto & index : indices_recv) {
    data_send.push_back(graph.vertex_attributes(index - start));
  }
  trace.exit_state("get components");
  trace.enter_state("send components");
    dart_alltoallv(data_send.data(), sizes_send_data.data(), 
        displs_send_data.data(), DART_TYPE_BYTE, data_recv.data(), 
        sizes_recv_data.data(), displs_recv_data.data(), 
        graph.team().dart_id());
  trace.exit_state("send components");

  trace.enter_state("create map");
  std::unordered_map<int, vprop_t> output;
  output.reserve(total_send);
  for(int i = 0; i < data_recv.size(); ++i) {
    output[indices_send[i]] = data_recv[i];
  }
  trace.exit_state("create map");
  return output;
}

template<typename GraphType>
void mst_set_data_min(
    matrix_min_pairs_t<typename GraphType::vertex_properties_type> & data_pairs, 
    int start,
    GraphType & graph,
    dash::util::Trace & trace
) {
  trace.enter_state("send pairs");
  typedef typename GraphType::vertex_properties_type      vprop_t;
  std::vector<std::size_t> sizes_send(data_pairs.size());
  std::vector<std::size_t> displs_send(data_pairs.size());
  std::vector<tuple_t<vprop_t>> pairs_send;
  std::size_t total_send = 0;
  for(int i = 0; i < data_pairs.size(); ++i) {
    sizes_send[i] = data_pairs[i].size() * sizeof(tuple_t<vprop_t>);
    displs_send[i] = total_send * sizeof(tuple_t<vprop_t>);
    total_send += data_pairs[i].size();
  }
  pairs_send.reserve(total_send);
  for(auto & pair_set : data_pairs) {
    pairs_send.insert(pairs_send.end(), pair_set.begin(), pair_set.end());
  }
  std::vector<std::size_t> sizes_recv(data_pairs.size());
  std::vector<std::size_t> displs_recv(data_pairs.size());
  dart_alltoall(sizes_send.data(), sizes_recv.data(), sizeof(std::size_t), 
      DART_TYPE_BYTE, graph.team().dart_id());
  std::size_t total_recv = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }
  std::vector<tuple_t<vprop_t>> pairs_recv(total_recv / 
      sizeof(tuple_t<vprop_t>));
  dart_alltoallv(pairs_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_BYTE, pairs_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());
  std::unordered_map<int, tuple_t<vprop_t>> mapping;
  for(auto & pair : pairs_recv) {
    auto it = mapping.find(std::get<0>(pair));
    if(it != mapping.end()) {
      auto weight = std::get<2>(it->second);
      if(weight > std::get<2>(pair)) {
        it->second = pair;
      }
    } else {
      mapping[std::get<0>(pair)] = pair;
    }
  }
  trace.exit_state("send pairs");
  trace.enter_state("set components");
  for(auto & pair : mapping) {
    graph.set_vertex_attributes(pair.first - start, std::get<1>(pair.second));
    if(dash::myid() == get<3>(pair.second)) {
      typename GraphType::edge_properties_type eprop { -1 };
      graph.set_out_edge_attributes(std::get<4>(pair.second), eprop);
    }
  }
  trace.exit_state("set components");
}

} // namespace internal

// TODO: component type should be Graph::vertex_size_type
/**
 * Computes minimum spanning tree on a graph.
 * Requires the graph's vertices to store the following attributes:
 * - (int) comp
 * Requires the graph's edges to store the following attributep:
 * - (int) weight
 * 
 * Output:
 * TODO: store the tree information in the edges OR in a separate data 
 *       structure
 */
template<typename GraphType>
void minimum_spanning_tree(GraphType & g) {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  dash::util::Trace trace("MinimumSpanningTree");
  // set component to global index in iteration space
  trace.enter_state("vertex setup");
  int i = 0;
  auto myid = dash::myid();
  auto git = g.vertex_gptr(g.vertices().lbegin());
  auto start = git.pos();
  for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
    auto index = it.pos(); 
    auto gindex = index + start;
    vprop_t prop { gindex, myid };
    g.set_vertex_attributes(index, prop);
    ++i;
  }
  trace.exit_state("vertex setup");

  trace.enter_state("barrier");
  dash::barrier();
  trace.exit_state("barrier");

  matrix_t indices(g.team().size());
  matrix_t permutations(g.team().size());
  {
    trace.enter_state("compute indices");
    int i = 0;
    for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
      auto v = g[it];
      for(auto e_it = v.out_edges().lbegin(); e_it != v.out_edges().lend(); 
          ++e_it) {
        auto trg = g[e_it].target();
        auto lpos = trg.lpos();
        indices[lpos.unit].push_back(lpos.index);
        permutations[lpos.unit].push_back(i);
        ++i;
      }
    }
    trace.exit_state("compute indices");
  }

  while(1) {
    int gr = 0;
    {
      std::vector<vprop_t> data = internal::mst_get_data(indices, permutations, 
          g, trace);
      trace.enter_state("compute pairs");
      matrix_min_pairs_t<vprop_t> data_pairs(g.team().size());
      std::vector<std::unordered_set<int>> pair_set(g.team().size());
      int i = 0;
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        auto src_comp = v.attributes();
        int min_weight = std::numeric_limits<int>::max();
        vprop_t trg_comp_min;
        int ledgepos = -1;
        for(auto e_it = v.out_edges().lbegin(); e_it != v.out_edges().lend(); 
            ++e_it) {
          auto e = g[e_it];
          auto e_weight = e.attributes().weight;
          auto trg_comp = data[i];
          if(src_comp.comp != trg_comp.comp && e_weight >= 0 && 
              min_weight > e_weight) {
            min_weight = e_weight;
            trg_comp_min = trg_comp;
            ledgepos = e_it.pos();
          }
          ++i;
        }
        if(ledgepos >= 0) {
          auto & set = pair_set[src_comp.unit];
          if(set.find(src_comp.comp) == set.end()) {
            data_pairs[src_comp.unit].push_back(
                std::make_tuple(src_comp.comp, trg_comp_min, min_weight, myid, 
                  ledgepos)
            );
            data_pairs[trg_comp_min.unit].push_back(
                std::make_tuple(trg_comp_min.comp, src_comp, min_weight, myid, 
                  ledgepos)
            );
            set.insert(src_comp.comp);
          }
          gr = 1;
        }
      }
      trace.exit_state("compute pairs");
      internal::mst_set_data_min(data_pairs, start, g, trace);
    }

    int gr_all = 0;
    trace.enter_state("allreduce data");
    dart_allreduce(&gr, &gr_all, 1, DART_TYPE_INT, DART_OP_SUM, 
        g.team().dart_id());
    trace.exit_state("allreduce data");
    if(gr_all == 0) break; 
    while(1) {
      int pj = 0;
      matrix_t indices(g.team().size());
      {
        std::vector<std::unordered_set<int>> comp_set(g.team().size());
        for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
          auto c = g[it].attributes();
          auto & set = comp_set[c.unit];
          if(set.find(c.comp) == set.end()) {
            indices[c.unit].push_back(c.comp);
            set.insert(c.comp);
          }
        }
      }
      auto components = internal::mst_get_components(indices, start, g, trace);

      trace.enter_state("set data (pj)");
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        auto comp = v.attributes().comp;
        if(comp != 0) {
          auto comp_next = components[comp];
          if(comp > comp_next.comp) {
            v.set_attributes(comp_next);
            pj = 1;
          }
        }
      }
      trace.exit_state("set data (pj)");
      int pj_all = 0;
      trace.enter_state("allreduce pointerjumping");
      dart_allreduce(&pj, &pj_all, 1, DART_TYPE_INT, DART_OP_SUM, 
          g.team().dart_id());
      trace.exit_state("allreduce pointerjumping");
      if(pj_all == 0) break; 
    }
  }

  dash::barrier();

}
} // namespace dash

#endif // DASH__ALGORITHM____GRAPH__MINIMUM_SPANNING_TREE_H__
