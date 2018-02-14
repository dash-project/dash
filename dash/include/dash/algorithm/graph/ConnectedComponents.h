#ifndef DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__
#define DASH__ALGORITHM____GRAPH__CONNECTED_COMPONENTS_H__

#include <dash/Graph.h>
#include <dash/util/Trace.h>

namespace dash {

typedef std::vector<std::vector<int>>                 matrix_t;
typedef std::vector<std::vector<std::pair<int, int>>> matrix_pair_t;

namespace internal {

template<typename GraphType>
std::vector<int> cc_get_data(
    matrix_t & indices, 
    matrix_t & permutations, 
    GraphType & graph,
    dash::util::Trace & trace) 
{
  trace.enter_state("send indices");
  // exchange indices to get
  std::vector<std::size_t> sizes_send(indices.size());
  std::vector<std::size_t> displs_send(indices.size());
  std::vector<int> indices_send;
  int total_send = 0;
  for(int i = 0; i < indices.size(); ++i) {
    sizes_send[i] = indices[i].size();
    displs_send[i] = total_send;
    total_send += indices[i].size();
  }
  indices_send.reserve(total_send);
  for(auto & index_set : indices) {
    indices_send.insert(indices_send.end(), index_set.begin(), 
        index_set.end());
  }
  std::vector<std::size_t> sizes_recv(indices.size());
  std::vector<std::size_t> displs_recv(indices.size());
  dart_alltoall(sizes_send.data(), sizes_recv.data(), sizeof(std::size_t), 
      DART_TYPE_BYTE, graph.team().dart_id());
  int total_recv = 0;
  for(int i = 0; i < sizes_recv.size(); ++i) {
    displs_recv[i] = total_recv;
    total_recv += sizes_recv[i];
  }
  std::vector<int> indices_recv(total_recv);
  if(total_send > 0 || total_recv > 0) {
    dart_alltoallv(indices_send.data(), sizes_send.data(), displs_send.data(),
        DART_TYPE_INT, indices_recv.data(), sizes_recv.data(), 
        displs_recv.data(), graph.team().dart_id());
  }

  trace.exit_state("send indices");
  // exchange data
  trace.enter_state("get components");
  for(auto & index : indices_recv) {
    // TODO: optimize cache performance
    index = graph[graph.vertices().lbegin() + index].attributes().comp;
  }
  trace.exit_state("get components");
  trace.enter_state("send components");
  if(total_send > 0 || total_recv > 0) {
    dart_alltoallv(indices_recv.data(), sizes_recv.data(), displs_recv.data(),
        DART_TYPE_INT, indices_send.data(), sizes_send.data(), 
        displs_send.data(), graph.team().dart_id());
  }
  trace.exit_state("send components");

  trace.enter_state("restore order");
  // restore original order
  // TODO: use more sophisticated ordering mechanism
  std::vector<int> output(indices_send.size());
  int index = 0;
  for(int i = 0; i < permutations.size(); ++i) {
    for(int j = 0; j < permutations[i].size(); ++j) {
      output[permutations[i][j]] = indices_send[index];
      ++index;
    }
  }
  trace.exit_state("restore order");
  return output;
}

template<typename GraphType>
void cc_set_data(matrix_pair_t & data_pairs, GraphType & graph, 
    dash::util::Trace & trace) {
  trace.enter_state("send pairs");
  std::vector<std::size_t> sizes_send(data_pairs.size());
  std::vector<std::size_t> displs_send(data_pairs.size());
  std::vector<std::pair<int, int>> pairs_send;
  int total_send = 0;
  for(int i = 0; i < data_pairs.size(); ++i) {
    sizes_send[i] = data_pairs[i].size() * sizeof(std::pair<int, int>);
    displs_send[i] = total_send * sizeof(std::pair<int, int>);
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
  std::vector<std::pair<int, int>> pairs_recv(total_recv / 
      sizeof(std::pair<int, int>));
  if(total_send > 0 || total_recv > 0) {
  dart_alltoallv(pairs_send.data(), sizes_send.data(), displs_send.data(),
      DART_TYPE_BYTE, pairs_recv.data(), sizes_recv.data(), 
      displs_recv.data(), graph.team().dart_id());
  }
  trace.exit_state("send pairs");
  trace.enter_state("set components");
  for(auto & pair : pairs_recv) {
    typename GraphType::vertex_properties_type prop { pair.second };
    graph[graph.vertices().lbegin() + pair.first].set_attributes(prop);
  }
  trace.exit_state("set components");
}

} // namespace internal

// TODO: component type should be Graph::vertex_size_type
/**
 * Computes connected components on a graph.
 * Requires the graph's vertices to store the following attributes:
 * - (int) comp
 * Requires the graph's edges to store the following attributep:
 * none
 * 
 * Output:
 * The vertex component attributes store the respective components after
 */
template<typename GraphType>
void connected_components(GraphType & g) {
  dash::util::Trace trace("ConnectedComponents");
  // set component to global index in iteration space
  // TODO: find faster method for this
  trace.enter_state("vertex setup");
  int i = 0;
  for(auto it = g.vertices().begin(); it != g.vertices().end(); ++it) {
    if(it.is_local()) {
      typename GraphType::vertex_properties_type prop { it.pos() };
      g[it].set_attributes(prop);
    }
    ++i;
  }
  trace.exit_state("vertex setup");

  trace.enter_state("barrier");
  dash::barrier();
  trace.exit_state("barrier");

  while(1) {
    int gr = 0;
    std::vector<int> data;
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

    {
      trace.enter_state("compute pairs");
      matrix_pair_t data_pairs(g.team().size());
      int i = 0;
      for(auto it = g.out_edges().lbegin(); it != g.out_edges().lend(); ++it) {
        auto e = g[it];
        auto src = g[e.source()];
        auto src_comp = src.attributes().comp;
        auto trg_comp = data[i];
        if(src_comp < trg_comp) {
          auto trg_comp_it = g.vertices().begin() + trg_comp;
          auto lpos = trg_comp_it.lpos();
           data_pairs[lpos.unit].push_back(
               std::make_pair(lpos.index, src_comp)
          );
          gr = 1;
        }
        ++i;
      }
      trace.exit_state("compute pairs");
      internal::cc_set_data(data_pairs, g, trace);
    }
    int gr_all = 0;
    trace.enter_state("allreduce data");
    dart_allreduce(&gr, &gr_all, 1, DART_TYPE_INT, DART_OP_SUM, 
        g.team().dart_id());
    trace.exit_state("allreduce data");
    if(gr_all == 0) break; 
    while(1) {
      int pj = 0;
      {
        trace.enter_state("compute indices (pj)");
        matrix_t indices(g.team().size());
        matrix_t permutations(g.team().size());
        int i = 0;
        for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
          auto v = g[it];
          auto comp = v.attributes().comp;
          auto next_it = g.vertices().begin() + comp;
          auto lpos = next_it.lpos();
          indices[lpos.unit].push_back(lpos.index);
          permutations[lpos.unit].push_back(i);
          ++i;
        }
        trace.exit_state("compute indices (pj)");
        data = internal::cc_get_data(indices, permutations, g, trace);
      }

      trace.enter_state("set data (pj)");
      int i = 0;
      for(auto it = g.vertices().lbegin(); it != g.vertices().lend(); ++it) {
        auto v = g[it];
        auto comp = v.attributes().comp;
        auto comp_next = data[i];
        if(comp != comp_next) {
          typename GraphType::vertex_properties_type prop { comp_next };
          v.set_attributes(prop);
          pj = 1;
        }
        ++i;
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
