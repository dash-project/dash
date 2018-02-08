#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <dash/graph/VertexIterator.h>
#include <dash/graph/EdgeIterator.h>
#include <dash/graph/InEdgeIterator.h>
#include <dash/graph/OutEdgeIterator.h>
#include <dash/graph/internal/Graph.h>
#include <dash/memory/GlobHeapContiguousMem.h>
#include <dash/memory/GlobHeapCombinedMem.h>
#include <dash/Team.h>
#include <dash/internal/Math.h>

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
  typename VertexProperties = EmptyProperties,  // user-defined struct
  typename EdgeProperties   = EmptyProperties,  // user-defined struct
  typename VertexSizeType  = int,
  typename EdgeSizeType    = int,
  template<class, class...> typename EdgeContainer = std::vector,
  template<class, class...> typename VertexContainer = std::vector
>
class Graph {

public:

  typedef Graph<Direction, 
          VertexProperties, EdgeProperties,
          VertexSizeType, EdgeSizeType, 
          EdgeContainer, VertexContainer>             self_t;
  typedef Vertex<self_t>                              vertex_type;
  typedef Edge<self_t>                                edge_type;
  typedef VertexContainer<vertex_type>                vertex_container_type;
  typedef EdgeContainer<edge_type>                    edge_container_type;

private:

  // TODO: add wrapper for all iterator types
  typedef VertexIteratorWrapper<self_t>               vertex_it_wrapper;
  typedef EdgeIteratorWrapper<self_t>                 edge_it_wrapper;
  typedef InEdgeIteratorWrapper<self_t>               in_edge_it_wrapper;
  typedef OutEdgeIteratorWrapper<self_t>              out_edge_it_wrapper;

  friend vertex_it_wrapper;
  friend edge_it_wrapper;
  friend in_edge_it_wrapper;
  friend out_edge_it_wrapper;

  typedef GlobHeapContiguousMem<
    vertex_container_type>                        glob_mem_vert_type;             
  typedef GlobHeapContiguousMem<                                                  
    edge_container_type>                          glob_mem_edge_type;             
  typedef                                                                         
    GlobHeapCombinedMem<glob_mem_edge_type>       glob_mem_edge_comb_type;        
  typedef std::vector<std::list<edge_type>>       edge_list_type;                 
                                                                                  
public:                                                                           
                                                                                  
  typedef typename                                                                
    glob_mem_vert_type::bucket_index_type         vertex_cont_ref_type;           
  typedef typename                                                                
    glob_mem_edge_type::bucket_index_type         edge_cont_ref_type;             
  typedef VertexSizeType                          vertex_size_type;               
  typedef EdgeSizeType                            edge_size_type;                 
                                                                                  
  typedef VertexProperties                        vertex_properties_type;         
  typedef EdgeProperties                          edge_properties_type;           
                                                                                  
  typedef GlobRef<vertex_type>                    reference;                      
                                                                                  
  typedef VertexIndex<vertex_size_type>           vertex_index_type;              
                                                                                  
  typedef typename                                                                
    glob_mem_vert_type::local_iterator            local_vertex_iterator;          
  typedef typename                                                                
    glob_mem_edge_type::local_iterator            local_in_edge_iterator;         
  typedef typename                                                                
    glob_mem_edge_type::local_iterator            local_out_edge_iterator;        
  typedef local_out_edge_iterator                 local_inout_edge_iterator;      
  typedef typename                                                                
    glob_mem_edge_comb_type::local_iterator       local_edge_iterator;            
                                                                                  
  typedef typename                                                                
    glob_mem_vert_type::global_iterator           global_vertex_iterator;         
  typedef typename                                                                
    glob_mem_edge_type::global_iterator           global_in_edge_iterator;        
  typedef typename                                                                
    glob_mem_edge_type::global_iterator           global_out_edge_iterator;       
  typedef global_out_edge_iterator                global_inout_edge_iterator;     
  typedef typename                                     
    glob_mem_edge_comb_type::global_iterator      global_edge_iterator;
                                                       

  typedef VertexProxy<self_t, local_vertex_iterator>                     
    local_vertex_proxy_type;
  typedef VertexProxy<self_t, global_vertex_iterator>                     
    global_vertex_proxy_type;
  typedef EdgeProxy<self_t, local_inout_edge_iterator>                       
    local_inout_edge_proxy_type;
  typedef EdgeProxy<self_t, global_inout_edge_iterator>                       
    global_inout_edge_proxy_type;
  typedef EdgeProxy<self_t, local_edge_iterator>                       
    local_edge_proxy_type;
  typedef EdgeProxy<self_t, global_edge_iterator>                       
    global_edge_proxy_type;

  friend local_vertex_proxy_type;
  friend global_vertex_proxy_type;
  friend local_inout_edge_proxy_type;
  friend global_inout_edge_proxy_type;
  friend local_edge_proxy_type;
  friend global_edge_proxy_type;

public:

  /**
   * Constructs an empty graph.
   */
  Graph(
      const vertex_size_type n_vertices = 0, 
      const edge_size_type n_vertex_edges = 0, 
      Team & team  = dash::Team::All()
  ) 
    : _team(&team),
      _myid(team.myid()),
      _remote_edges(team.size())
  {
    allocate(n_vertices, n_vertex_edges);
  }

  /**
   * Constructs a graph from an iterator range pointing to elements of type
   * std::pair<vertex_size_type, vertex_size_type>.
   * Assumes vertex IDs are taken from a contiguous range [0...n] and n is
   * divisible by the number of units in the Team of the container.
   * Partitions vertices based on their id:
   * vertex_id / (n / num_units) = owner
   * 
   * /todo Add 
   */
  template<typename ForwardIterator>
  Graph(
      ForwardIterator begin, 
      ForwardIterator end, 
      vertex_size_type n_vertices, 
      Team & team = dash::Team::All()
  )
    : _team(&team),
      _myid(team.myid()),
      _remote_edges(team.size())
  {
    // TODO: find heuristic to allocate a reasonable amount of edge memory 
    //       per vertex
    allocate(n_vertices, 0);

    std::map<vertex_size_type, local_vertex_iterator> lvertices;
    std::map<vertex_size_type, global_vertex_iterator> gvertices;
    // TODO: find method that uses less memory
    std::unordered_set<vertex_size_type> remote_vertices_set;
    std::vector<std::vector<vertex_size_type>> remote_vertices(_team->size());
    for(auto it = begin; it != end; ++it) {
      auto v = it->first;
      if(vertex_owner(v, n_vertices) == _myid) {
        auto u = it->second;

        if(lvertices.find(v) == lvertices.end()) {
          // add dummy first, more vertices are added later and they have
          // to be in order
          lvertices[v] = local_vertex_iterator();
        }
        auto target_owner = vertex_owner(u, n_vertices);
        if(target_owner == _myid) {
          if(lvertices.find(u) == lvertices.end()) {
            // add dummy first, more vertices are added later and they have
            // to be in order
            lvertices[u] = local_vertex_iterator();
          }
        } else {
          // collect vertices for remote nodes and prevent adding vertices more
          // than once
          bool inserted;
          std::tie(std::ignore, inserted) = remote_vertices_set.insert(u);
          if(inserted) {
            remote_vertices[target_owner].push_back(u);
          }
        }
      }
    }

    // send vertices to their owner units and receive their local index
    {
      std::vector<std::size_t> sizes_send(remote_vertices.size());
      std::vector<std::size_t> sizes_send_n(remote_vertices.size());
      std::vector<std::size_t> displs_send(remote_vertices.size());
      std::vector<vertex_size_type> remote_vertices_send;
      int total_send = 0;
      for(int i = 0; i < remote_vertices.size(); ++i) {
        sizes_send[i] = remote_vertices[i].size() * sizeof(vertex_size_type);
        sizes_send_n[i] = remote_vertices[i].size();
        displs_send[i] = total_send * sizeof(vertex_size_type);
        total_send += remote_vertices[i].size();
      }
      remote_vertices_send.reserve(total_send);
      for(auto & vertex_set : remote_vertices) {
        remote_vertices_send.insert(remote_vertices_send.end(), 
            vertex_set.begin(), vertex_set.end());
      }
      std::vector<std::size_t> sizes_recv(remote_vertices.size());
      std::vector<std::size_t> displs_recv(remote_vertices.size());
      dart_alltoall(sizes_send.data(), sizes_recv.data(), sizeof(std::size_t), 
          DART_TYPE_BYTE, _team->dart_id());
      int total_recv = 0;
      for(int i = 0; i < sizes_recv.size(); ++i) {
        displs_recv[i] = total_recv;
        total_recv += sizes_recv[i];
      }
      std::vector<vertex_size_type> remote_vertices_recv(total_recv / 
        sizeof(vertex_size_type));
      if(total_send > 0 || total_recv > 0) {
        dart_alltoallv(remote_vertices_send.data(), sizes_send.data(), 
            displs_send.data(), DART_TYPE_BYTE, remote_vertices_recv.data(), 
            sizes_recv.data(), displs_recv.data(), _team->dart_id());
      }

      // exchange data
      for(auto & index : remote_vertices_recv) {
          lvertices[index] = local_vertex_iterator();
      }
      // add vertices in order
      for(auto & lvertex : lvertices) {
        lvertex.second = add_vertex();
      }
      // send local position of added vertices that are referenced by edges 
      // on other units
      for(auto & index : remote_vertices_recv) {
        if(lvertices.find(index) != lvertices.end()) {
          index = lvertices[index].pos();
        }
      }
      if(total_send > 0 || total_recv > 0) {
        dart_alltoallv(remote_vertices_recv.data(), sizes_recv.data(), 
            displs_recv.data(), DART_TYPE_BYTE, remote_vertices_send.data(), 
            sizes_send.data(), displs_send.data(), _team->dart_id());
      }
      // all vertices have been added - commit changes to global memory space
      commit();
      // remote_vertices_send now contains the local indices in the iteration 
      // space of the corresponding unit
      team_unit_t unit { 0 };
      int index = 0;
      for(auto & lindex : remote_vertices_send) {
        while(index >= sizes_send_n[unit]) {
          ++unit;
          index = 0;
          if(unit >= _team->size()) {
            break;
          }
        }
        if(unit >= _team->size()) {
          break;
        }
        gvertices[remote_vertices[unit][index]] = global_vertex_iterator(
            _glob_mem_vertex, unit, lindex);
        ++index;
      }
    }

    // finally add edges with the vertex iterators gained from the previous
    // steps
    for(auto it = begin; it != end; ++it) {
      auto v = it->first;
      if(vertex_owner(v, n_vertices) == _myid) {
        auto v_it = lvertices[it->first];
        auto u = it->second;

        if(vertex_owner(u, n_vertices) == _myid) {
          auto u_it = lvertices[u];
          add_edge(v_it, u_it);
        } else {
          auto u_it = gvertices[u];
          add_edge(v_it, u_it);
        }
      }
    }
    // commit edges
    commit();
  }

  /** Destructs the graph.
   */
  ~Graph() {
    deallocate();
  }

  /**
   * Copy-constructor. Explicitly deleted.
   */
  Graph(const self_t &) = delete;

  /**
   * Move-constructor. Explicitly deleted.
   */
  Graph(self_t &&) = delete;

  /**
   * Copy-assignment operator. Explicitly deleted.
   */
  self_t & operator=(const self_t &) = delete;

  /**
   * Move-assignment operator. Explicitly deleted.
   */
  self_t & operator=(self_t &&) = delete;

  /**
   * Returns an object handling interactions with a vertex pointed to by
   * the given iterator.
   */
  local_vertex_proxy_type operator[](local_vertex_iterator it) {
    return local_vertex_proxy_type(it, this);
  }

  /**
   * Returns an object handling interactions with a vertex pointed to by
   * the given iterator.
   */
  global_vertex_proxy_type operator[](global_vertex_iterator it) {
    return global_vertex_proxy_type(it, this);
  }

  /**
   * Returns an object handling interactions with an edge pointed to by
   * the given iterator.
   */
  local_inout_edge_proxy_type operator[](local_inout_edge_iterator it) {
    return local_inout_edge_proxy_type(it, this);
  }

  /**
   * Returns an object handling interactions with an edge pointed to by
   * the given iterator.
   */
  global_inout_edge_proxy_type operator[](global_inout_edge_iterator it) {
    return global_inout_edge_proxy_type(it, this);
  }

  /**
   * Returns an object handling interactions with an edge pointed to by
   * the given iterator.
   */
  global_edge_proxy_type operator[](global_edge_iterator it) {
    return global_edge_proxy_type(it, this);
  }

  /**
   * Returns a vertex iterator range object.
   */
  vertex_it_wrapper & vertices() {
    return _vertices;
  }

  /**
   * Returns an edge iterator range object.
   */
  edge_it_wrapper & edges() {
    return _edges;
  }
  
  /**
   * Returns an in-edge iterator range object.
   */
  in_edge_it_wrapper & in_edges() {
    return _in_edges;
  }

  /**
   * Returns an out-edge iterator range object.
   */
  out_edge_it_wrapper & out_edges() {
    return _out_edges;
  }

  /**
   * Returns the number of vertices in the whole graph.
   * 
   * \return The amount of vertices in the whole graph.
   */
  vertex_size_type num_vertices() const {
    return _glob_mem_vertex->size();
  }

  /**
   * Returns the number of edges in the whole graph.
   * 
   * \return The amount of edges in the whole graph.
   */
  edge_size_type num_edges() const {
    return _glob_mem_edge->size();
  }

  /**
   * Returns, whether the graph is empty.
   * 
   * \return True, if the graph holds 0 vertices. Fals otherwise.
   */
  bool empty() const {
    return _glob_mem_vertex->size() == 0 ? true : false;
  }

  /**
   * Adds a vertex with the given properties locally.
   * 
   * \return Index of the newly created vertex.
   */
  local_vertex_iterator add_vertex(const VertexProperties & prop) {
    _glob_mem_out_edge->add_container(_alloc_edges_per_vertex);
    if(_glob_mem_in_edge != _glob_mem_out_edge) {
      _glob_mem_in_edge->add_container(_alloc_edges_per_vertex);
    }
    vertex_type v(prop);
    return _glob_mem_vertex->push_back(_vertex_container_ref, v);
  }

  /**
   * Adds a vertex locally.
   * 
   * \return Index of the newly created vertex.
   */
  local_vertex_iterator add_vertex() {
    VertexProperties prop;
    return add_vertex(prop);
  }

  /**
   * Removes a given vertex.
   */
  void remove_vertex(const local_vertex_iterator & v) {
    
  }

  /**
   * Removes a given vertex.
   */
  void remove_vertex(const global_vertex_iterator & v) {
    
  }

  /**
   * Adds an edge between two given vertices with the given properties 
   * locally.
   * 
   * \return Pair, with pair::first set to the index of the newly created edge
   *         and pair::second set to a boolean indicating whether the edge has
   *         actually been added.
   */
  std::pair<local_out_edge_iterator, bool> add_edge(
      const local_vertex_iterator & source, 
      const local_vertex_iterator & target, 
      const EdgeProperties & prop
  ) {
    local_out_edge_iterator l_it;
    l_it = add_local_edge(source, target, prop, _glob_mem_out_edge);
    add_local_edge(target, source, prop, _glob_mem_in_edge);

    // currently, double edges are allowed for all cases, and vertex deletion 
    // is not implemented so we always return true
    return std::make_pair(l_it, true);
  }

  /**
   * Adds an edge between two given vertices with the given properties 
   * locally.
   * Edges that belong to vertices held on a different unit are marked for
   * transfer. These edges will be transferred after calling \c commit().
   * 
   * \return Pair, with pair::first set to the index of the newly created edge
   *         and pair::second set to a boolean indicating whether the edge has
   *         actually been added.
   */
  std::pair<local_out_edge_iterator, bool> add_edge(
      const local_vertex_iterator & source, 
      const global_vertex_iterator & target, 
      const EdgeProperties & prop
  ) {
    local_out_edge_iterator l_it;
    l_it = add_local_edge(source, target, prop, _glob_mem_out_edge);
    if(target.is_local()) {
      // _glob_mem_in_edge == _glob_mem_out_edge for undirected graph types
      add_local_edge(target.local(), source, prop, _glob_mem_in_edge);
    // do not double-send edges
    } else {
      edge_type edge(source, target, prop, _myid);
      _remote_edges[target.lpos().unit].push_back(edge);
    }
    
    // currently, double edges are allowed for all cases, and vertex deletion 
    // is not implemented so we always return true
    return std::make_pair(l_it, true);
  }

  /**
   * Adds an edge between two given vertices locally.
   * Edges that belong to vertices held on a different unit are marked for
   * transfer. These edges will be transferred after calling \c commit().
   * 
   * \return Pair, with pair::first set to the index of the newly created edge
   *         and pair::second set to a boolean indicating whether the edge has
   *         actually been added.
   */
  std::pair<local_out_edge_iterator, bool> add_edge(
      const local_vertex_iterator & source, 
      const local_vertex_iterator & target 
  ) {
    EdgeProperties prop;
    return add_edge(source, target, prop);
  }

  /**
   * Adds an edge between two given vertices locally.
   * Edges that belong to vertices held on a different unit are marked for
   * transfer. These edges will be transferred after calling \c commit().
   * 
   * \return Pair, with pair::first set to the index of the newly created edge
   *         and pair::second set to a boolean indicating whether the edge has
   *         actually been added.
   */
  std::pair<local_out_edge_iterator, bool> add_edge(
      const local_vertex_iterator & source, 
      const global_vertex_iterator & target 
  ) {
    EdgeProperties prop;
    return add_edge(source, target, prop);
  }

  /**
   * Removes the edges between two given vertices.
   */
  void remove_edge(const local_vertex_iterator & v1, 
      const local_vertex_iterator & v2) {

  }

  /**
   * Removes the edges between two given vertices.
   */
  void remove_edge(const global_vertex_iterator & v1, 
      const global_vertex_iterator & v2) {

  }

  /**
   * Removes a given edge.
   */
  void remove_edge(const local_out_edge_iterator & e) {

  }

  /**
   * Removes a given edge.
   */
  void remove_edge(const global_out_edge_iterator & e) {

  }

  /**
   * Commits local changes of the graph to global memory space since the last
   * call of this method.
   */
  void commit() {
    // move all edges that have to be added by other units in a contiguous 
    // memory region
    std::vector<edge_type> remote_edges;
    std::vector<std::size_t> remote_edges_count(_team->size());
    std::vector<std::size_t> remote_edges_displs(_team->size());
    for(int i = 0; i < _remote_edges.size(); ++i) {
      for(auto & remote_edge : _remote_edges[i]) {
        remote_edges.push_back(remote_edge);
        remote_edges_count[i] += sizeof(edge_type);
      }
      for(int j = i + 1; j < remote_edges_displs.size(); ++j) {
        remote_edges_displs[j] += remote_edges_count[i];
      }
    _remote_edges[i].clear();
    }
    // exchange amount of edges to be transferred with other units
    std::vector<std::size_t> edge_count(_team->size());
    DASH_ASSERT_RETURNS(
      dart_alltoall(remote_edges_count.data(), edge_count.data(), 
          sizeof(std::size_t), DART_TYPE_BYTE, _team->dart_id()),
      DART_OK
    );
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
    DASH_ASSERT_RETURNS(
      dart_alltoallv(remote_edges.data(), 
          remote_edges_count.data(), 
          remote_edges_displs.data(), 
          DART_TYPE_BYTE, 
          edges.data(), 
          edge_count.data(), 
          edge_displs.data(), 
          _team->dart_id()
      ),
      DART_OK
    );
    // add missing edges to local memory space
    for(auto edge : edges) {
      if(edge.source.unit == _myid) {
        add_local_edge(edge.source, edge.target, edge.properties, 
            _glob_mem_out_edge);
      }
      if(edge.target.unit == _myid) {
        // _glob_mem_in_edge == _glob_mem_out_edge for undirected graph types
        add_local_edge(edge.target, edge.source, edge.properties, 
            _glob_mem_in_edge);
        //TODO: for directed graphs, should target and source really be mutated
        //      in the in-edge list?
      }
    }
    // commit changes in local memory space globally
    _glob_mem_vertex->commit();
    _glob_mem_out_edge->commit();
    if(_glob_mem_out_edge != _glob_mem_in_edge) {
      _glob_mem_in_edge->commit();
    }
    _glob_mem_edge->commit();
  }

  /**
   * Globally allocates memory for vertex and edge storage.
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
    return true;
  }

  /**
   * Deallocates global memory.
   */
  void deallocate() {
    if(_glob_mem_vertex != nullptr) {
      delete _glob_mem_vertex;
      _glob_mem_vertex = nullptr;
    }
    if(_glob_mem_in_edge != nullptr) {
      delete _glob_mem_in_edge;
      if(_glob_mem_in_edge == _glob_mem_out_edge) {
        _glob_mem_out_edge = nullptr;
      }
      _glob_mem_in_edge = nullptr;
    }
    if(_glob_mem_out_edge != nullptr 
        && _glob_mem_out_edge != _glob_mem_in_edge) {
      delete _glob_mem_out_edge;
      _glob_mem_out_edge = nullptr;
    }
    if(_glob_mem_edge != nullptr) {
      delete _glob_mem_edge;
      _glob_mem_edge = nullptr;
    }
    if(_team != nullptr) {
      // Remove deallocator from the respective team instance
      _team->unregister_deallocator(this, std::bind(&Graph::deallocate, this));
    }
  }

  /**
   * Returns the team containing all units associated with this container
   */
  Team & team() const {
    return *_team;
  }

  global_vertex_iterator vertex_gptr(local_vertex_iterator it) {
    return global_vertex_iterator(_glob_mem_vertex, _myid, it.pos());
  }

private:

  /**
   * Inserts an edge locally. The source vertex must belong to this unit.
   * 
   * \return Local iterator to the created edge.
   */
  template<typename TargetIterator>
  typename glob_mem_edge_type::local_iterator add_local_edge(      
      const local_vertex_iterator source, 
      const TargetIterator target, 
      const EdgeProperties & prop,
      glob_mem_edge_type * glob_mem_edge
  ) {
    auto edge = edge_type(source, target, prop, _myid);
    return glob_mem_edge->push_back(source.pos(), edge);
  }

  /**
   * Inserts an edge locally. The source vertex must belong to this unit.
   * 
   * \return Local iterator to the created edge.
   */
  typename glob_mem_edge_type::local_iterator add_local_edge(      
      const vertex_index_type source, 
      const vertex_index_type target, 
      const EdgeProperties & prop,
      glob_mem_edge_type * glob_mem_edge
  ) {
    auto edge = edge_type(source, target, prop);
    return glob_mem_edge->push_back(source.offset, edge);
  }

  team_unit_t vertex_owner(vertex_size_type v, vertex_size_type n_vertices) {
    int owner_id = static_cast<double>(v) / (static_cast<double>(n_vertices) / _team->size());
    team_unit_t owner { owner_id };
    return owner;
  }

private:

  /** the team containing all units using the container */
  Team *                      _team     = nullptr;
  /** Global memory allocation and access to vertices */
  glob_mem_vert_type *        _glob_mem_vertex = nullptr;
  /** Global memory allocation and access to inbound edges */
  glob_mem_edge_type *        _glob_mem_in_edge = nullptr;
  /** Global memory allocation and access to outbound edges */
  glob_mem_edge_type *        _glob_mem_out_edge = nullptr;
  /** Access to inbound and outbound edges */
  glob_mem_edge_comb_type *   _glob_mem_edge = nullptr;
  /** Unit ID of the current unit */
  team_unit_t                 _myid{DART_UNDEFINED_UNIT_ID};
  /** Index of the vertex container in _glob_mem_vertex */ 
  vertex_cont_ref_type        _vertex_container_ref;
  /** Amount of edge elements to be pre-allocated for every vertex */
  edge_size_type              _alloc_edges_per_vertex;
  /** Edges that have to be added to vertices on another unit in next commit */
  edge_list_type              _remote_edges;
  /** wrapper for vertex iterator ranges */
  vertex_it_wrapper           _vertices = vertex_it_wrapper(this);
  /** wrapper for edge iterator ranges */
  edge_it_wrapper             _edges = edge_it_wrapper(this);
  /** wrapper for in-edge iterator ranges */
  in_edge_it_wrapper          _in_edges = in_edge_it_wrapper(this);
  /** wrapper for out-edge iterator ranges */
  out_edge_it_wrapper         _out_edges = out_edge_it_wrapper(this);

};

}

#endif // DASH__GRAPH_H__INCLUDED

