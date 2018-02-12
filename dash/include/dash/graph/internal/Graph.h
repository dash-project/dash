#ifndef DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
#define DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED

namespace dash {

/**
 * Enum declaring the different graph types.
 */
enum GraphDirection {
  UndirectedGraph,
  DirectedGraph
  //BidirectionalGraph
};

/**
 * Index type for vertices.
 */
template <typename IndexType>
struct VertexIndex {

  /**
   * Default constructor.
   */
  VertexIndex() = default;

  /**
   * Index Constructor.
   */
  VertexIndex(team_unit_t u, IndexType o) 
    : unit(u),
      offset(o)
  { }

  /** The unit holding the referenced vertex */
  team_unit_t     unit;
  /** The offset of the vertex in local memory space of unit */
  IndexType       offset;

};

/**
 * Vertex type holding properties and references to its corresponding edge 
 * lists.
 */
template<typename GraphType>
class Vertex {

  typedef typename GraphType::vertex_properties_type   properties_type;

  friend GraphType;

public:

  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op.
   */
  Vertex() = default;
  
  /**
   * Creates a vertex with given properties.
   */
  Vertex(
      const properties_type & properties
  ) 
    : properties(properties)
  { }

public:

  /** Properties of this vertex */
  properties_type       properties;

};

/**
 * Proxy type for vertices. Encapsulates logic for data retrieval, data manipulation 
 * and iterator retrieval.
 */
template<typename GraphType, typename IteratorType>
class VertexProxy {

  typedef GraphType                                     graph_type;
  typedef IteratorType                                  iterator_type;
  typedef VertexProxy<GraphType, IteratorType>          self_t;
  typedef typename graph_type::vertex_type              vertex_type;
  typedef typename graph_type::vertex_properties_type   properties_type;
  typedef typename graph_type::global_vertex_iterator   global_vertex_iterator;
  typedef typename graph_type::local_vertex_iterator    local_vertex_iterator;

  /**
   * Handles the iterator ranges for adjacency iteration of the respective 
   * vertex.
   */
  template<typename Parent, typename GlobMemType>
  class edge_range {

    typedef Parent parent_type;
    typedef GlobMemType glob_mem_type;
    typedef typename glob_mem_type::index_type          index_type;
    typedef typename glob_mem_type::global_iterator     global_iterator;
    typedef typename glob_mem_type::local_iterator      local_iterator;
    typedef typename parent_type::graph_type::local_vertex_iterator
      local_vertex_iterator;
    typedef typename parent_type::graph_type::glob_mem_edge_comb_type
      glob_mem_edge_comb_type;

    public:

      /*
       * Constructs the range handler object.
       */
      edge_range(parent_type & p, glob_mem_type * gmem) 
        : _parent(p),
          _glob_mem(gmem)
      { }

      /**
       * Returns global iterator to the first element in the edge list of
       * the given vertex.
       */
      global_iterator begin() {
        return begin(_parent._iterator);
      }

      /**
       * Returns global iterator past the last element in the edge list of
       * the given vertex.
       */
      global_iterator end() {
        return end(_parent._iterator);
      }

      local_iterator lbegin() {
        return lbegin(_parent._iterator);
      }

      local_iterator lend() {
        return lend(_parent._iterator);
      }

    private:

      /**
       * Begin iterator for global vertex iterators.
       */
      template<typename VertexIteratorType>
      global_iterator begin(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        auto index = it_position(_glob_mem, lpos.index);
        return global_iterator(
            _glob_mem,
            lpos.unit,
            index,
            0
        );
      }

      /**
       * Begin iterator for local vertex iterators.
       */
      global_iterator begin(local_vertex_iterator it) {
        auto index = it_position(_glob_mem, it.pos());
        return global_iterator(
            _glob_mem,
            _parent._graph->_myid,
            index,
            0
        );
      }
      
      /**
       * End iterator for global vertex iterators.
       */
      template<typename VertexIteratorType>
      global_iterator end(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        auto index = it_position(_glob_mem, lpos.index);
        return global_iterator(
            _glob_mem,
            lpos.unit,
            index,
            _glob_mem->container_size(lpos.unit, index)
        );
      }

      /**
       * End iterator for local vertex iterators.
       */
      global_iterator end(local_vertex_iterator it) {
        auto index = it_position(_glob_mem, it.pos());
        return global_iterator(
            _glob_mem,
            _parent._graph->_myid,
            index,
            _glob_mem->container_size(_parent._graph->_myid, index)
        );
      }

      /**
       * Local begin iterator for global vertex iterators.
       */
      template<typename VertexIteratorType>
      local_iterator lbegin(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        auto index = it_position(_glob_mem, lpos.index);
        auto lindex = index * 2;
        auto bucket_it = _glob_mem->_buckets.begin();
        std::advance(bucket_it, lindex);
        // if bucket empty, advance to end iterator
        if(bucket_it->size == 0) {
          std::advance(bucket_it, 2);
        }
        auto position = 0;
        // TODO: position WRONG, if there are uncommitted changes
        //       save cumulated sizes of unattached containers somewhere
        if(index > 0) {
          position = 
            _glob_mem->_bucket_cumul_sizes[_parent._graph->_myid][index - 1];
        }
        // use lightweight constructor to avoid costly increment operations
        return local_iterator(
            _glob_mem->_buckets.begin(),
            _glob_mem->_buckets.end(),
            position,
            bucket_it,
            0
        );
      }

      /**
       * Local begin iterator for local vertex iterators.
       */
      local_iterator lbegin(local_vertex_iterator it) {
        auto index = it_position(_glob_mem, it.pos());
        auto lindex = index * 2;
        auto bucket_it = _glob_mem->_buckets.begin();
        std::advance(bucket_it, lindex);
        // if bucket empty, advance to end iterator
        if(bucket_it->size == 0) {
          std::advance(bucket_it, 2);
        }
        auto position = 0;
        // TODO: position WRONG, if there are uncommitted changes
        //       save cumulated sizes of unattached containers somewhere
        if(index > 0) {
          position = 
            _glob_mem->_bucket_cumul_sizes[_parent._graph->_myid][index - 1];
        }
        // use lightweight constructor to avoid costly increment operations
        return local_iterator(
            _glob_mem->_buckets.begin(),
            _glob_mem->_buckets.end(),
            position,
            bucket_it,
            0
        );
      }
      
      /**
       * Local end iterator for global vertex iterators.
       */
      template<typename VertexIteratorType>
      local_iterator lend(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        auto index = it_position(_glob_mem, lpos.index) + 1;
        auto lindex = index * 2;
        auto bucket_it = _glob_mem->_buckets.begin();
        //std::advance(bucket_it, lindex);
        auto position = 0;
        // TODO: position WRONG, if there are uncommitted changes
        //       save cumulated sizes of unattached containers somewhere
        if(index > 0) {
          position = 
            _glob_mem->_bucket_cumul_sizes[_parent._graph->_myid][index - 1];
        }
        // use lightweight constructor to avoid costly increment operations
        return local_iterator(
            _glob_mem->_buckets.begin(),
            _glob_mem->_buckets.end(),
            position,
            bucket_it,
            0
        );
      }

      /**
       * Local end iterator for local vertex iterators.
       */
      local_iterator lend(local_vertex_iterator it) {
        auto index = it_position(_glob_mem, it.pos()) + 1;
        auto lindex = index * 2;
        auto bucket_it = _glob_mem->_buckets.begin();
        //std::advance(bucket_it, index);
        auto position = 0;
        // TODO: position WRONG, if there are uncommitted changes
        //       save cumulated sizes of unattached containers somewhere
        if(index > 0) {
          position = 
            _glob_mem->_bucket_cumul_sizes[_parent._graph->_myid][index - 1];
        }
        // use lightweight constructor to avoid costly increment operations
        return local_iterator(
            _glob_mem->_buckets.begin(),
            _glob_mem->_buckets.end(),
            position,
            bucket_it,
            0
        );
      }

      /**
       * Determines the position of the edge-list for inbound and outbound
       * edge memory spaces.
       */
      template<typename _GMem>
      index_type it_position(_GMem * gmem, index_type pos) {
        return pos;
      }

      /**
       * Determines the position of the edge-list for the combined edge memory 
       * space.
       */
      index_type it_position(glob_mem_edge_comb_type * gmem, index_type pos) {
        return pos * 2;
      }

    private:

      /** Reference to the corresponding VertexProxy object */
      parent_type &    _parent;
      /** Reference to the GlobMem object of the targeted memory space */
      glob_mem_type *  _glob_mem;

  };

  typedef edge_range<self_t, typename graph_type::glob_mem_edge_type> 
    inout_edge_range_type;
  typedef edge_range<self_t, typename graph_type::glob_mem_edge_comb_type> 
    edge_range_type;

public:

  /**
   * Default constructor. Explicitly deleted.
   */
  VertexProxy() = delete;

  /**
   * Constructs the object with a vertex iterator.
   */
  VertexProxy(iterator_type it, graph_type * g) 
    : _iterator(it),
      _graph(g),
      _out_edges(*this, g->_glob_mem_out_edge),
      _in_edges(*this, g->_glob_mem_in_edge),
      _edges(*this, g->_glob_mem_edge)
  { }

  /**
   * Returns a range for adjacent outbound edges of the referenced vertex.
   */
  inout_edge_range_type & out_edges() {
    return _out_edges;
  }

  /**
   * Returns a range for adjacent inbound edges of the referenced vertex.
   */
  inout_edge_range_type & in_edges() {
    return _in_edges;
  }

  /**
   * Returns a range for adjacent edges of the referenced vertex.
   */
  edge_range_type & edges() {
    return _edges;
  }

  /**
   * Returns the properties of the referenced vertex. Data is loaded lazily.
   */
  properties_type & attributes() {
    // load properties lazily
    if(!_vertex_loaded) {
      _vertex = *_iterator;
      _vertex_loaded = true;
    }
    return _vertex.properties;
  }

  /**
   * Sets the attribute data for the referenced vertex.
   */
  void set_attributes(properties_type & prop) {
    set_attributes(prop, _iterator);
  }

private:

  /**
   * Overload of set_aatributes for global pointers.
   */
  void set_attributes(properties_type & prop, global_vertex_iterator it) {
    _vertex.properties = prop;
    auto ref = *_iterator;
    ref = _vertex;
  }

  /**
   * Overload of set_aatributes for local pointers.
   */
  void set_attributes(properties_type & prop, local_vertex_iterator it) {
    _vertex.properties = prop;
    auto ptr = _iterator;
    *ptr = _vertex;
  }

private:

  /** Iterator to the referenced vertex */
  iterator_type           _iterator;
  /** data of the referenced vertex */
  vertex_type             _vertex;
  /** Whether the vertex data has already been loaded */
  bool                    _vertex_loaded = false;
  /** Pointer to the graph container */
  graph_type *            _graph;
  /** Range object for outbound edges */
  inout_edge_range_type   _out_edges;
  /** Range object for inbbound edges */
  inout_edge_range_type   _in_edges;
  /** Range object for edges */
  edge_range_type         _edges;

};

/**
 * Edge type holding properties and references to the vertices it belongs to.
 */
template<typename GraphType>
class Edge {

  typedef typename GraphType::local_vertex_iterator    local_vertex_iterator;
  typedef typename GraphType::global_vertex_iterator   global_vertex_iterator;
  typedef typename GraphType::vertex_index_type        vertex_index_type;
  typedef typename GraphType::edge_properties_type     properties_type;

public:

  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op.
   */
  Edge() = default;

  /**
   * Creates an edge.
   */
  Edge(
      const local_vertex_iterator & source,
      const local_vertex_iterator & target, 
      const properties_type & properties,
      const team_unit_t owner
  ) 
    : source(owner, source.pos()),
      target(owner, target.pos()),
      properties(properties)
  { }

  /**
   * Creates an edge.
   */
  Edge(
      const local_vertex_iterator & source,
      const global_vertex_iterator & target, 
      const properties_type & properties,
      const team_unit_t owner
  )
    : source(owner, source.pos()),
      target(target.lpos().unit, target.lpos().index),
      properties(properties)
  { }

  /**
   * Creates an edge.
   */
  Edge(
      const vertex_index_type & source,
      const vertex_index_type & target, 
      const properties_type & properties
  )
    : source(source),
      target(target),
      properties(properties)
  { }

public:

  /** Properties of this edge */
  properties_type       properties;
  /** Source vertex the edge is pointing from */
  vertex_index_type     source;
  /** Target vertex the edge is pointing to */
  vertex_index_type     target;

};

/**
 * Proxy type for edges. Encapsulates logic for data retrieval and 
 * manipulation.
 */
template<typename GraphType, typename IteratorType>
class EdgeProxy {

  typedef GraphType                                          graph_type;
  typedef IteratorType                                       iterator_type;
  typedef typename graph_type::edge_type                     edge_type;
  typedef typename graph_type::edge_properties_type          properties_type;
  typedef typename graph_type::global_vertex_iterator        vertex_iterator;

public:
  /**
   * Default constructor. Explicitly deleted.
   */
  EdgeProxy() = delete;

  /**
   * Constructs the edge proxy from a given edge iterator.
   */
  EdgeProxy(iterator_type it, graph_type * g) 
    : _iterator(it),
      _graph(g)
  { }

  /**
   * Returns the properties of the referenced edge. Data is loaded lazily.
   */
  properties_type & attributes() {
    lazy_load();
    return _edge.properties;
  }

  /**
   * Returns iglobal iterator to the source vertex.
   */
  vertex_iterator source() {
    lazy_load();
    return vertex_iterator(
        _graph->_glob_mem_vertex,
        _edge.source.unit,
        _edge.source.offset
    );
  }

  /**
   * Returns iglobal iterator to the target vertex.
   */
  vertex_iterator target() {
    lazy_load();
    return vertex_iterator(
        _graph->_glob_mem_vertex,
        _edge.target.unit,
        _edge.target.offset
    );
  }

  /**
   * Sets the attribute data for the referenced edge.
   */
  void set_attributes(properties_type & prop) {
    lazy_load();
    _edge.properties = prop;
    auto ptr = _iterator;
    *ptr = _edge;
  }

private:

  /**
   * Loads edge data lazily.
   */
  void lazy_load() {
    if(!_edge_loaded) {
      _edge = *_iterator;
      _edge_loaded = true;
    }
  }

private:

  /** Iterator to the referenced edge */
  iterator_type         _iterator;
  /** Data of the referenced edge */
  edge_type             _edge;
  /** Whether edge data has already been loaded */
  bool                  _edge_loaded = false;
  /** Pointer to the graph container */
  graph_type *          _graph;

};

/**
 * Default property type holding no data.
 */
struct EmptyProperties { };

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
