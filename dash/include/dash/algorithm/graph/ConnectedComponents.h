#ifndef DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__
#define DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__

#include <dash/Graph.h>
#include <dash/util/Trace.h>

namespace dash {

typedef std::vector<std::vector<int>>                 matrix_t;
template<typename T> 
using matrix_pair_t = std::vector<std::vector<std::pair<int, T>>>;

namespace internal {
// TODO: merge overlapping code of the following functions with the functions
//       provided in MinimumSpanningTree.h

template<typename GraphType>
auto cc_get_data(
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
auto cc_get_components(
    matrix_t & indices, 
    int start,
    GraphType & graph,
    dash::util::Trace & trace
) -> std::pair<
       std::unordered_map<int, typename GraphType::vertex_properties_type>, 
       std::vector<std::vector<typename GraphType::vertex_properties_type>>> 
{
  typedef typename GraphType::vertex_properties_type      vprop_t;
  trace.enter_state("send indices");
  auto myid = graph.team().myid();
  std::size_t lsize = graph.vertex_size(myid);
  std::vector<int> thresholds(indices.size());
  for(int i = 0; i < thresholds.size(); ++i) {
    team_unit_t unit { i };
    // TODO: find a threshold that provides an optimal tradeoff for any 
    //       amount of units
    thresholds[i] = graph.vertex_size(unit) * 20;
  }

  std::vector<long long> total_sizes(indices.size());
  std::vector<std::size_t> sizes_send(indices.size());
  std::vector<long long> sizes_send_longdt(indices.size());
  std::vector<std::size_t> displs_send(indices.size());
  std::vector<std::size_t> sizes_recv_data(indices.size());
  std::vector<std::size_t> cumul_sizes_recv_data(indices.size());
  std::vector<std::size_t> displs_recv_data(indices.size());
  for(int i = 0; i < indices.size(); ++i) {
    sizes_send[i] = indices[i].size();
    sizes_send_longdt[i] = indices[i].size();
    sizes_recv_data[i] = indices[i].size() * sizeof(vprop_t);
  }
  dart_allreduce(sizes_send_longdt.data(), total_sizes.data(), indices.size(), 
      DART_TYPE_LONGLONG, DART_OP_SUM, graph.team().dart_id());
  int total_send = 0;
  int total_recv_data = 0;
  for(int i = 0; i < indices.size(); ++i) {
    if(total_sizes[i] > thresholds[i]) {
      sizes_send[i] = 0;
      team_unit_t unit { i };
      sizes_recv_data[i] = graph.vertex_size(unit) * sizeof(vprop_t);
      indices[i].clear();
    } else {
      sizes_send[i] = indices[i].size();
      sizes_recv_data[i] = indices[i].size() * sizeof(vprop_t);
    }
    displs_send[i] = total_send;
    displs_recv_data[i] = total_recv_data;
    total_send += sizes_send[i];
    total_recv_data += sizes_recv_data[i];
    cumul_sizes_recv_data[i] = total_recv_data / sizeof(vprop_t); 
  }
  std::vector<int> indices_send;
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
  int total_send_data = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    if(total_sizes[myid] > thresholds[myid]) {
      sizes_send_data[i] = lsize * sizeof(vprop_t);
      displs_send_data[i] = 0;
    } else {
      sizes_send_data[i] = sizes_recv[i] * sizeof(vprop_t);
      displs_send_data[i] = total_send_data;
      displs_recv[i] = total_recv;
    }
    total_recv += sizes_recv[i];
    total_send_data += sizes_send_data[i];
  }
  std::vector<int> indices_recv(total_recv);
  dart_alltoallv(indices_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_INT, indices_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());
  trace.exit_state("send indices");
  trace.enter_state("get components");
  std::vector<vprop_t> data_send;
  std::vector<vprop_t> data_recv(total_recv_data / sizeof(vprop_t));
  if(total_sizes[myid] > thresholds[myid]) {
    data_send.reserve(lsize);
    for(int i = 0; i < lsize; ++i) {
      data_send.push_back(graph.vertex_attributes(i));
    }
  } else {
    data_send.reserve(total_recv);
    int i = 0;
    for(auto & index : indices_recv) {
      data_send.push_back(graph.vertex_attributes(index - start));
      ++i;
    }
  }
  trace.exit_state("get components");
  trace.enter_state("send components");
  dart_alltoallv(data_send.data(), sizes_send_data.data(), 
      displs_send_data.data(), DART_TYPE_BYTE, data_recv.data(), 
      sizes_recv_data.data(), displs_recv_data.data(), 
      graph.team().dart_id());
  trace.exit_state("send components");
  trace.enter_state("create map");
  std::unordered_map<int, vprop_t>  output_regular;
  std::vector<std::vector<vprop_t>> output_contiguous(indices.size());
  output_regular.reserve(total_send);
  int current_unit = 0;
  int j = 0;
  for(int i = 0; i < data_recv.size(); ++i) {
    if(i >= cumul_sizes_recv_data[current_unit]) {
      ++current_unit;
    }
    if(total_sizes[current_unit] > thresholds[current_unit]) {
      output_contiguous[current_unit].push_back(data_recv[i]);
    } else {
      output_regular[indices_send[j]] = data_recv[i];
      ++j;
    }
  }
  trace.exit_state("create map");
  return std::make_pair(output_regular, output_contiguous);
}

template<typename GraphType>
void cc_set_data(
    matrix_pair_t<typename GraphType::vertex_properties_type> & data_pairs, 
    int start,
    GraphType & graph, 
    dash::util::Trace & trace
) {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  trace.enter_state("send pairs");
  std::vector<std::size_t> sizes_send(data_pairs.size());
  std::vector<std::size_t> displs_send(data_pairs.size());
  std::vector<std::pair<int, vprop_t>> pairs_send;
  int total_send = 0;
  for(int i = 0; i < data_pairs.size(); ++i) {
    sizes_send[i] = data_pairs[i].size() * sizeof(std::pair<int, vprop_t>);
    displs_send[i] = total_send * sizeof(std::pair<int, vprop_t>);
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
  std::vector<std::pair<int, vprop_t>> pairs_recv(total_recv / 
      sizeof(std::pair<int, vprop_t>));
  dart_alltoallv(pairs_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_BYTE, pairs_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());
  trace.exit_state("send pairs");
  trace.enter_state("set components");
  for(auto & pair : pairs_recv) {
    int ind = pair.first - start;
    graph.set_vertex_attributes(pair.first - start, pair.second);
  }
  trace.exit_state("set components");
}

} // namespace internal

// TODO: component type should be Graph::vertex_size_type
/**
 * Computes connected components on a graph.
 * Requires the graph's vertices to store the following attributes:
 * - (int) comp
 * - (int) unit
 * Requires the graph's edges to store the following attributep:
 * none
 * 
 * Output:
 * The vertex component attributes store the respective components after
 */
template<typename GraphType>
void connected_components(GraphType & g) {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  dash::util::Trace trace("ConnectedComponents");
  // set component to global index in iteration space
  // TODO: find faster method for this
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

  std::vector<int> unit_sizes(g.team().size());
  std::size_t total_size = 0;
  for(int i = 0; i < g.team().size(); ++i) {
    unit_sizes[i] = total_size;
    team_unit_t unit { i };
    total_size += g.vertex_size(unit);
  }

  matrix_t indices(g.team().size());
  matrix_t permutations(g.team().size());
  {
    trace.enter_state("compute indices");
    int i = 0;
    for(auto it = g.out_edges().lbegin(); it != g.out_edges().lend(); ++it) {
      auto trg = g[it].target();
      auto lpos = trg.lpos();
      // TODO: use more sophsticated sorting mechanism
      indices[lpos.unit].push_back(lpos.index);
      permutations[lpos.unit].push_back(i);
      ++i;
    }
    trace.exit_state("compute indices");
  }

  while(1) {
    int gr = 0;
    {
      std::vector<vprop_t> data = internal::cc_get_data(indices, permutations, 
          g, trace);
      trace.enter_state("compute pairs");
      matrix_pair_t<vprop_t> data_pairs(g.team().size());
      {
        std::vector<std::unordered_set<int>> pair_set(g.team().size());
        int i = 0;
        for(auto it = g.out_edges().lbegin(); it != g.out_edges().lend(); ++it) {
          if(data[i].comp != 0) {
            auto e = g[it];
            auto src = g[e.source()];
            auto src_comp = src.attributes();
            auto trg_comp = data[i];
            if(src_comp.comp < trg_comp.comp) {
              auto & set = pair_set[trg_comp.unit];
              if(set.find(trg_comp.comp) == set.end()) {
                data_pairs[trg_comp.unit].emplace_back(trg_comp.comp, src_comp);
                set.insert(trg_comp.comp);
              }
              gr = 1;
            }
          }
          ++i;
        }
      }
      trace.exit_state("compute pairs");
      internal::cc_set_data(data_pairs, start, g, trace);
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
      auto components = internal::cc_get_components(indices, start, g, trace);

      trace.enter_state("set data (pj)");
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        auto comp = v.attributes();
        if(comp.comp != 0) {
          vprop_t comp_next;
          if(components.second[comp.unit].size() > 0) {
            comp_next = 
              components.second[comp.unit][comp.comp - unit_sizes[comp.unit]];
          } else {
            comp_next = components.first[comp.comp];
          }
          if(comp.comp != comp_next.comp) {
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

#endif // DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__

