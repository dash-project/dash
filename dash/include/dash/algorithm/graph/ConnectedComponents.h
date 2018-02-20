#ifndef DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__
#define DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__

#include <dash/Graph.h>
#include <dash/util/Trace.h>

namespace dash {

typedef std::vector<std::vector<int>>                 matrix_t;
template<typename T> 
using matrix_pair_t = std::vector<std::vector<std::pair<int, T>>>;

namespace internal {

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
  if(total_send > 0 || total_recv > 0) {
    dart_alltoallv(indices_send.data(), sizes_send.data(), displs_send.data(),
        DART_TYPE_INT, indices_recv.data(), sizes_recv.data(), 
        displs_recv.data(), graph.team().dart_id());
  }

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
  if(total_send > 0 || total_recv > 0) {
    dart_alltoallv(data_send.data(), sizes_send_data.data(), 
        displs_send_data.data(), DART_TYPE_BYTE, data_recv.data(), 
        sizes_recv_data.data(), displs_recv_data.data(), 
        graph.team().dart_id());
  }
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
auto cc_get_components(
    GraphType & graph,
    dash::util::Trace & trace
) -> std::vector<typename GraphType::vertex_properties_type> {
  typedef typename GraphType::vertex_properties_type      vprop_t;
  std::size_t size = graph.local_vertex_size() * sizeof(vprop_t);
  std::size_t size_n = graph.local_vertex_size();
  std::vector<std::size_t> sizes_recv(graph.team().size());
  std::vector<std::size_t> displs_recv(graph.team().size());
  int total_recv = 0;
  dart_allgather(&size, sizes_recv.data(), sizeof(std::size_t), 
      DART_TYPE_BYTE, graph.team().dart_id());

  for(int i = 0; i < sizes_recv.size(); ++i) {
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }

  std::vector<vprop_t> components_send;
  components_send.reserve(size_n);
  for(int i = 0; i < size_n; ++i) {
    components_send.push_back(graph.vertex_attributes(i));
  }

  std::vector<vprop_t> components_recv(total_recv / sizeof(vprop_t));
  dart_allgatherv(components_send.data(), size, DART_TYPE_BYTE, 
    components_recv.data(), sizes_recv.data(), displs_recv.data(), 
    graph.team().dart_id());

  return components_recv;
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
  int total_recv = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }
  std::vector<std::pair<int, vprop_t>> pairs_recv(total_recv / 
      sizeof(std::pair<int, vprop_t>));
  if(total_send > 0 || total_recv > 0) {
  dart_alltoallv(pairs_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_BYTE, pairs_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());
  }
  trace.exit_state("send pairs");
  trace.enter_state("set components");
  for(auto & pair : pairs_recv) {
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

  while(1) {
    int gr = 0;
    {
      std::vector<vprop_t> data;
      {
        trace.enter_state("compute indices");
        matrix_t indices(g.team().size());
        matrix_t permutations(g.team().size());
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
        data = internal::cc_get_data(indices, permutations, g, trace);
      }

      trace.enter_state("compute pairs");
      matrix_pair_t<vprop_t> data_pairs(g.team().size());
      int i = 0;
      for(auto it = g.out_edges().lbegin(); it != g.out_edges().lend(); ++it) {
        if(data[i].comp != 0) {
          auto e = g[it];
          auto src = g[e.source()];
          auto src_comp = src.attributes();
          auto trg_comp = data[i];
          if(src_comp.comp < trg_comp.comp) {
             data_pairs[trg_comp.unit].push_back(
                 std::make_pair(trg_comp.comp, src_comp)
            );
            gr = 1;
          }
        }
        ++i;
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
      std::vector<vprop_t> components = internal::cc_get_components(g, trace);

      trace.enter_state("set data (pj)");
      int i = 0;
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        auto comp = v.attributes().comp;
        if(comp != 0) {
          auto comp_next = components[comp];
          if(comp != comp_next.comp) {
            v.set_attributes(comp_next);
            pj = 1;
          }
          ++i;
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
