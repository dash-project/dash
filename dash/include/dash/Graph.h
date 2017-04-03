#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

#include <vector>
#include <dash/graph/VertexIterator.h>
#include <dash/graph/EdgeIterator.h>
#include <dash/graph/internal/Graph.h>
#include <dash/GlobDynamicContiguousMem.h>
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
  typedef internal::vertex<graph_type>                vertex_type;
  typedef internal::out_edge<graph_type>              edge_type;
  typedef VertexContainer<vertex_type, 
          std::allocator<vertex_type>>                vertex_container_type;
  typedef EdgeContainer<edge_type, 
          std::allocator<edge_type>>                  edge_container_type;

private:

  // TODO: add wrapper for all iterator types
  typedef VertexIteratorWrapper<graph_type>           vertex_it_wrapper;
  typedef EdgeIteratorWrapper<graph_type>             edge_it_wrapper;
  typedef VertexIteratorWrapper<graph_type>           in_edge_it_wrapper;
  typedef VertexIteratorWrapper<graph_type>           out_edge_it_wrapper;
  typedef VertexIteratorWrapper<graph_type>           adjacency_it_wrapper;

  friend vertex_it_wrapper;
  friend edge_it_wrapper;

  typedef GlobDynamicContiguousMem<
    vertex_container_type>                            glob_mem_vert_type;
  typedef GlobDynamicContiguousMem<
    edge_container_type>                              glob_mem_edge_type;
  typedef std::vector<std::list<edge_type>>           edge_list_type;

public:

  typedef typename 
    glob_mem_vert_type::container_list_index          vertex_cont_ref_type;
  typedef typename 
    glob_mem_edge_type::container_list_index          edge_cont_ref_type;
  typedef VertexIndexType                             vertex_offset_type;
  typedef EdgeIndexType                               edge_offset_type;
  typedef internal::VertexIndex<VertexIndexType>      vertex_index_type;
  typedef internal::EdgeIndex<EdgeIndexType>          edge_index_type;
  typedef typename 
    std::make_unsigned<VertexIndexType>::type         vertex_size_type;
  typedef typename 
    std::make_unsigned<EdgeIndexType>::type           edge_size_type;

  typedef DynamicPattern                              pattern_type;
  typedef VertexProperties                            vertex_properties_type;
  typedef EdgeProperties                              edge_properties_type;

  typedef GlobRef<vertex_type>                        reference;

  typedef typename 
    glob_mem_vert_type::local_iterator                local_vertex_iterator;
  typedef typename 
    glob_mem_edge_type::local_iterator                local_edge_iterator;

  typedef typename glob_mem_vert_type::global_iterator global_vertex_iterator;
  typedef typename glob_mem_edge_type::global_iterator global_edge_iterator;
  
  typedef typename vertex_it_wrapper::iterator        vertex_iterator;
  typedef typename edge_it_wrapper::iterator          edge_iterator;
  typedef typename in_edge_it_wrapper::iterator       in_edge_iterator;
  typedef typename out_edge_it_wrapper::iterator      out_edge_iterator;
  typedef typename adjacency_it_wrapper::iterator     adjacency_iterator;

public:

  vertex_it_wrapper      vertices = vertex_it_wrapper(this);
  edge_it_wrapper        edges = edge_it_wrapper(this);
  in_edge_it_wrapper     in_edges = in_edge_it_wrapper(this);
  out_edge_it_wrapper    out_edges = out_edge_it_wrapper(this);
  adjacency_it_wrapper   adjacent_vertices = adjacency_it_wrapper(this);

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
    ++_local_vertex_max_index;
    auto ref = _glob_mem_edge->add_container(_alloc_edges_per_vertex);
    vertex_type v(_local_vertex_max_index, ref, prop);
    _glob_mem_vertex->push_back(_vertex_container_ref, v);
    // TODO: return global index
    return vertex_index_type(_myid, _local_vertex_max_index);
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
  std::pair<edge_index_type, bool> add_edge(const vertex_index_type & v1, 
      const vertex_index_type & v2, 
      const EdgeProperties & prop = EdgeProperties()) {
    edge_index_type local_index(_myid, 0, -1);
    //TODO: Handle errors, if vertices do not exist
    auto edge1 = edge_type(0, v1, v2, prop);
    if(v1.unit == _myid) {
      auto vertex1 = _glob_mem_vertex->get(_vertex_container_ref, v1.offset);
      edge1._local_id = ++_local_edge_max_index;
      auto local_offset = _glob_mem_edge->push_back(vertex1._edge_ref, edge1);
      local_index.unit = v1.unit;
      local_index.container = vertex1._edge_ref;
      local_index.offset = local_offset;
    } else {
      //TODO: check, if unit ID is valid
      _remote_edges[v1.unit].push_back(edge1);
    }
    auto edge2 = edge_type(0, v2, v1, prop);
    if(v2.unit == _myid) {
      auto vertex2 = _glob_mem_vertex->get(_vertex_container_ref, v2.offset);
      edge2._local_id = ++_local_edge_max_index;
      auto local_offset = _glob_mem_edge->push_back(vertex2._edge_ref, edge2);
      if(local_index.offset == -1) {
        local_index.unit = v2.unit;
        local_index.container = vertex2._edge_ref;
        local_index.offset = local_offset;
      }
    } else {
      _remote_edges[v2.unit].push_back(edge2);
    }
    //TODO: Check, whether the edge already exists
    //TODO: if feasible, use pointer to prop, so it gets saved only once
    //TODO: find out, how to save edges for different graph types 
    //      (directed, undirected)
    //TODO: Find a way to return index, if both vertices reside on different
    //      unit. 
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
      auto vertex = _glob_mem_vertex->get(_vertex_container_ref, 
          edge._source.offset);
      edge._local_id = ++_local_edge_max_index;
      _glob_mem_edge->push_back(vertex._edge_ref, edge);
    }
    // commit changes in local memory space globally
    _glob_mem_vertex->commit();
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
    _glob_mem_edge = new glob_mem_edge_type(*_team);
    // Register deallocator at the respective team instance
    _team->register_deallocator(this, std::bind(&Graph::deallocate, this));
  }

  /**
   * Deallocates global memory of this container.
   */
  void deallocate() {
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

  /** the team containing all units using the container */
  Team *                      _team     = nullptr;
  /** Global memory allocation and access for sequential memory regions */
  glob_mem_vert_type *        _glob_mem_vertex = nullptr;

  glob_mem_edge_type *        _glob_mem_edge = nullptr;
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

