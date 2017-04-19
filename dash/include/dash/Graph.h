#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

#include <vector>
#include <dash/graph/VertexIterator.h>
#include <dash/graph/EdgeIterator.h>
#include <dash/graph/InEdgeIterator.h>
#include <dash/graph/OutEdgeIterator.h>
#include <dash/graph/internal/Graph.h>
#include <dash/GlobDynamicContiguousMem.h>
#include <dash/GlobDynamicCombinedMem.h>
#include <dash/Team.h>
#include <dash/internal/Math.h>
#include <dash/graph/GlobGraphIter.h>

namespace dash {

/**
 * \defgroup DashGraphConcept Graph Concept
 * Concept of a distributed graph container.
 * 
 * \ingroup DashContainerConcept
 * \{
 * \psr Description
 * 
 * Description here.
 * 
 * Extends Concepts:
 * 
 * Container properties:
 * 
 * Iterator validity:
 * 
 * \par Member types
 * 
 * \par Member functions
 * 
 * \}
 * 
 * Usage examples:
 * 
 */


/**
 * Distributed, dynamic graph container for sparse graphs.
 */
template<
  GraphDirection Direction  = DirectedGraph,
  //typename DynamicPattern   = dash::graph::VertexPartitionedDynamicPattern,
  typename DynamicPattern   = void,
  typename VertexProperties = internal::EmptyProperties,  // user-defined struct
  typename EdgeProperties   = internal::EmptyProperties,  // user-defined struct
  typename VertexIndexType  = int,
  typename EdgeIndexType    = int,
  template<class, class> typename EdgeContainer = std::vector,
  template<class, class> typename VertexContainer = std::vector
>
class Graph {

public:

  typedef Graph<Direction, DynamicPattern, 
          VertexProperties, EdgeProperties,
          VertexIndexType, EdgeIndexType, 
          EdgeContainer, VertexContainer>             graph_type;
  typedef internal::Vertex<graph_type>                vertex_type;
  typedef internal::Edge<graph_type>                  edge_type;
  typedef VertexContainer<vertex_type, 
          std::allocator<vertex_type>>                vertex_container_type;
  typedef EdgeContainer<edge_type, 
          std::allocator<edge_type>>                  edge_container_type;

private:

  // TODO: add wrapper for all iterator types
  typedef VertexIteratorWrapper<graph_type>           vertex_it_wrapper;
  typedef EdgeIteratorWrapper<graph_type>             edge_it_wrapper;
  typedef InEdgeIteratorWrapper<graph_type>           in_edge_it_wrapper;
  typedef OutEdgeIteratorWrapper<graph_type>          out_edge_it_wrapper;

  friend vertex_it_wrapper;
  friend edge_it_wrapper;
  friend in_edge_it_wrapper;
  friend out_edge_it_wrapper;

  typedef GlobDynamicContiguousMem<
    vertex_container_type>                          glob_mem_vert_type;
  typedef GlobDynamicContiguousMem<
    edge_container_type>                            glob_mem_edge_type;
  typedef 
    GlobDynamicCombinedMem<glob_mem_edge_type>      glob_mem_edge_comb_type;
  typedef std::vector<std::list<edge_type>>         edge_list_type;

public:

  typedef typename 
    glob_mem_vert_type::container_list_index        vertex_cont_ref_type;
  typedef typename 
    glob_mem_edge_type::container_list_index        edge_cont_ref_type;
  typedef VertexIndexType                           vertex_offset_type;
  typedef EdgeIndexType                             edge_offset_type;
  typedef internal::VertexIndex<VertexIndexType>    vertex_index_type;
  typedef internal::EdgeIndex<EdgeIndexType>        edge_index_type;
  typedef typename 
    std::make_unsigned<VertexIndexType>::type       vertex_size_type;
  typedef typename 
    std::make_unsigned<EdgeIndexType>::type         edge_size_type;

  typedef DynamicPattern                            pattern_type;
  typedef VertexProperties                          vertex_properties_type;
  typedef EdgeProperties                            edge_properties_type;

  typedef GlobRef<vertex_type>                      reference;

  typedef typename 
    glob_mem_vert_type::local_iterator              local_vertex_iterator;
  typedef typename 
    glob_mem_edge_type::local_iterator              local_edge_iterator;

  typedef typename 
    glob_mem_vert_type::global_iterator             global_vertex_iterator;
  typedef typename 
    glob_mem_edge_type::global_iterator             global_edge_iterator;
  typedef typename 
    glob_mem_edge_comb_type::global_iterator        global_edge_comb_iterator;
  
  typedef typename vertex_it_wrapper::iterator      vertex_iterator;
  typedef typename edge_it_wrapper::iterator        edge_iterator;
  typedef typename in_edge_it_wrapper::iterator     in_edge_iterator;
  typedef typename out_edge_it_wrapper::iterator    out_edge_iterator;

public:

  vertex_it_wrapper      vertices = vertex_it_wrapper(this);
  edge_it_wrapper        edges = edge_it_wrapper(this);
  in_edge_it_wrapper     in_edges = in_edge_it_wrapper(this);
  out_edge_it_wrapper    out_edges = out_edge_it_wrapper(this);

public:

  /**
   * Constructs an empty graph.
   */
  Graph(
      vertex_size_type n_vertices = 0, 
      edge_size_type n_vertex_edges = 0, 
      Team & team  = dash::Team::All()
  ) 
    : _team(&team),
      _myid(team.myid()),
      _remote_edges(team.size())
  {
    allocate(n_vertices, n_vertex_edges);
  }

  /** Destructs the graph.
   */
  ~Graph() {
    deallocate();
  }

  /**
   * Returns the number of vertices in the whole graph.
   */
  const vertex_size_type num_vertices() const {

  }

  /**
   * Returns the index of the vertex with the highest index in the whole graph.
   */
  const vertex_index_type max_vertex_index() const {

  }

  /**
   * Returns the number of edges in the whole graph.
   */
  const edge_size_type num_edges() const {

  }

  /**
   * Returns the index of the edge with the highest index in the whole graph.
   */
  const edge_index_type max_edge_index() const {

  }

  /**
   * Returns, whether the graph is empty.
   */
  bool empty() const {
    return true;
  }

  /**
   * Adds a vertex with the given properties.
   * NOTE: global method.
   */
  vertex_index_type add_vertex(const VertexProperties & prop 
      = VertexProperties()) {
    auto in_ref = _glob_mem_in_edge->add_container(_alloc_edges_per_vertex);
    auto out_ref = _glob_mem_out_edge->add_container(_alloc_edges_per_vertex);
    auto max_index = _glob_mem_vertex->container_local_size(
        _vertex_container_ref);
    vertex_index_type v_index(_myid, max_index);
    vertex_type v(v_index, in_ref, out_ref, prop);
    _glob_mem_vertex->push_back(_vertex_container_ref, v);
    // TODO: return global index
    return vertex_index_type(_myid, max_index);
  }

  /**
   * Removes a given vertex.
   * NOTE: global method.
   */
  void remove_vertex(const vertex_index_type & v) {
    
  }

  /**
   * Removes all edges (in & out) from the given vertex).
   * NOTE: global method.
   */
  void clear_vertex(vertex_index_type & v) {

  }

  /**
   * Adds an edge between two given vertices with the given properties 
   * locally.
   * NOTE: global method.
   */
  std::pair<edge_index_type, bool> add_edge(
      const vertex_index_type & source, 
      const vertex_index_type & target, 
      const EdgeProperties & prop = EdgeProperties()
  ) {
    edge_index_type local_index;
    if(source.unit == _myid) {
      local_index = add_local_edge(source, target, prop, _glob_mem_out_edge);
    } else {
      edge_type edge(source, target, prop);
      _remote_edges[source.unit].push_back(edge);
    }
    if(target.unit == _myid) {
      // _glob_mem_in_edge == _glob_mem_out_edge for undirected graph types
      edge_index_type local_index_tmp = add_local_edge(target, 
            source, prop, _glob_mem_in_edge);
      if(local_index.offset == -1) {
        local_index = local_index_tmp;
      }
    // do not double-send edges
    } else if(source.unit != target.unit) {
      edge_type edge(source, target, prop);
      _remote_edges[target.unit].push_back(edge);
    }
    //TODO: handle cases, were both vertices reside on a different unit
    
    // currently, double edges are allowed for all cases, so we always return 
    // true
    return std::make_pair(local_index, true);
  }

  /**
   * Removes an edge between two given vertices.
   * NOTE: global method.
   */
  void remove_edge(const vertex_index_type & v1, 
      const vertex_index_type & v2) {

  }

  /**
   * Removes a given edge.a9f5c03d9de8dda4
   * NOTE: global method.
   */
  void remove_edge(const edge_index_type & e) {

  }

  /**
   * Commits local changes performed by methods classified as "global" to
   * the whole data structure.
   */
  void barrier() {
    // move all edges that have to be added by other units in a contiguous 
    // memory region
    std::vector<edge_type> remote_edges;
    std::vector<std::size_t> remote_edges_count(_team->size());
    std::vector<std::size_t> remote_edges_displs(_team->size());
    for(int i = 0; i < _remote_edges.size(); ++i) {
      for(auto remote_edge : _remote_edges[i]) {
        remote_edges.push_back(remote_edge);
        remote_edges_count[i] += sizeof(edge_type);
      }
      for(int j = i + 1; j < remote_edges_displs.size(); ++j) {
        remote_edges_displs[j] += remote_edges_count[i];
      }
    }
    _remote_edges.clear();
    // exchange amount of edges to be transferred with other units
    std::vector<std::size_t> edge_count(_team->size());
    dart_alltoall(remote_edges_count.data(), edge_count.data(), 
        sizeof(std::size_t), DART_TYPE_BYTE, _team->dart_id());
    int total_count = 0;
    std::vector<std::size_t> edge_displs(_team->size());
    for(int i = 0; i < edge_count.size(); ++i) {
      total_count += edge_count[i];
      for(int j = i + 1; j < edge_displs.size(); ++j) {
        edge_displs[j] += edge_count[i];
      }
    }
    // exchange edges
    std::vector<edge_type> edges(total_count / sizeof(edge_type));
    dart_alltoallv(remote_edges.data(), 
        remote_edges_count.data(), 
        remote_edges_displs.data(), 
        DART_TYPE_BYTE, 
        edges.data(), 
        edge_count.data(), 
        edge_displs.data(), 
        _team->dart_id()
    );
    // add missing edges to local memory space
    for(auto edge : edges) {
      if(edge._source.unit == _myid) {

        add_local_edge(edge._source, edge._target, edge.properties, 
            _glob_mem_out_edge);
      }
      if(edge._target.unit == _myid) {
        // _glob_mem_in_edge == _glob_mem_out_edge for undirected graph types
        add_local_edge(edge._target, edge._source, edge.properties, 
            _glob_mem_in_edge);
      }
    }
    // commit changes in local memory space globally
    _glob_mem_vertex->commit();
    _glob_mem_out_edge->commit();
    _glob_mem_in_edge->commit();
    _glob_mem_edge->commit();
  }

  /**
   * Globally allocates memory for vertex storage.
   */
  bool allocate(vertex_size_type n_vertices, edge_size_type n_vertex_edges) {
    auto vertex_lcap = dash::math::div_ceil(n_vertices, _team->size());
    _glob_mem_vertex = new glob_mem_vert_type(*_team);
    _vertex_container_ref = _glob_mem_vertex->add_container(vertex_lcap);
    // no edge list allocation yet, this will happen once the vertices are 
    // created. Each edge list will have n_vertex_edges elements reserved.
    _alloc_edges_per_vertex = n_vertex_edges;
    _glob_mem_out_edge = new glob_mem_edge_type(*_team);
    if(Direction == DirectedGraph) {
      _glob_mem_in_edge = new glob_mem_edge_type(*_team);
    } else {
      // there is no distinction between in- and out-edges in an undirected 
      // graph
      _glob_mem_in_edge = _glob_mem_out_edge;
    }
    _glob_mem_edge = new glob_mem_edge_comb_type(*_team);
    _glob_mem_edge->add_globmem(*_glob_mem_out_edge);
    _glob_mem_edge->add_globmem(*_glob_mem_in_edge);
    // Register deallocator at the respective team instance
    _team->register_deallocator(this, std::bind(&Graph::deallocate, this));
  }

  /**
   * Deallocates global memory of this container.
   */
  void deallocate() {
    // TODO: Delete all objects created on the heap, and look for objects with
    //       shared ownership
    if(_glob_mem_vertex != nullptr) {
      delete _glob_mem_vertex;
    }
    // Remove deallocator from the respective team instance
    _team->unregister_deallocator(this, std::bind(&Graph::deallocate, this));
  }

  vertex_type test() {
    auto gptr = _glob_mem_vertex->dart_gptr_at(team_unit_t(1), 0, 0);
    return reference(gptr);
  }

private:

  edge_index_type add_local_edge(      
      const vertex_index_type & source, 
      const vertex_index_type & target, 
      const EdgeProperties & prop,
      glob_mem_edge_type * glob_mem_edge
  ) {
    edge_index_type local_index(_myid, 0, -1);
    auto edge = edge_type(source, target, prop);
    auto vertex = _glob_mem_vertex->get(_vertex_container_ref, 
        source.offset);
    auto edge_ref = vertex._out_edge_ref;
    if(glob_mem_edge == _glob_mem_in_edge) {
      edge_ref = vertex._in_edge_ref;
    }
    local_index = edge_index_type(
        _myid, 
        edge_ref, 
        glob_mem_edge->container_local_size(edge_ref)
    );
    edge._index = local_index;
    glob_mem_edge->push_back(edge_ref, edge);
    return local_index;
  }

private:

  /** the team containing all units using the container */
  Team *                      _team     = nullptr;
  /** Global memory allocation and access for sequential memory regions */
  glob_mem_vert_type *        _glob_mem_vertex = nullptr;

  glob_mem_edge_type *        _glob_mem_in_edge = nullptr;
  
  glob_mem_edge_type *        _glob_mem_out_edge = nullptr;

  glob_mem_edge_comb_type *   _glob_mem_edge = nullptr;
  /** Unit ID of the current unit */
  team_unit_t                 _myid{DART_UNDEFINED_UNIT_ID};
  /** Index of last added vertex */
  vertex_offset_type          _local_vertex_max_index = -1;

  edge_offset_type            _local_edge_max_index = -1;
  /** Iterator to the vertex container in GlobMem object */ 
  vertex_cont_ref_type        _vertex_container_ref;

  edge_size_type              _alloc_edges_per_vertex;
  /** edges that have to be added to vertices on another unit */
  edge_list_type              _remote_edges;

};

}

#endif // DASH__GRAPH_H__INCLUDED

