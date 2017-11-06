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

template<typename GraphType, typename IteratorType>
class VertexProxy {

  typedef GraphType                                     graph_type;
  typedef IteratorType                                  iterator_type;
  typedef VertexProxy<GraphType, IteratorType>          self_t;
  typedef typename GraphType::vertex_type               vertex_type;
  typedef typename GraphType::vertex_properties_type    properties_type;

  template<typename Parent, typename GlobMemType>
  class edge_range {

    typedef Parent parent_type;
    typedef GlobMemType glob_mem_type;
    typedef typename glob_mem_type::global_iterator     global_iterator;
    typedef typename glob_mem_type::local_iterator      local_iterator;
    typedef typename parent_type::graph_type::local_vertex_iterator
      local_vertex_iterator;

    public:

      edge_range(parent_type & p, glob_mem_type * gmem) 
        : _parent(p),
          _glob_mem(gmem)
      { }

      global_iterator begin() {
        return begin(_parent._iterator);
      }

      global_iterator end() {
        return end(_parent._iterator);
      }

    private:

      template<typename VertexIteratorType>
      global_iterator begin(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        return global_iterator(
            _glob_mem,
            lpos.unit,
            lpos.index,
            0
        );
      }

      global_iterator begin(local_vertex_iterator it) {
        return global_iterator(
            _glob_mem,
            _parent._graph->_myid,
            it.pos(),
            0
        );
      }
      
      template<typename VertexIteratorType>
      global_iterator end(VertexIteratorType it) {
        auto lpos = _parent._iterator.lpos();
        return global_iterator(
            _glob_mem,
            lpos.unit,
            lpos.index,
            _glob_mem->container_size(lpos.unit, lpos.index)
        );
      }

      global_iterator end(local_vertex_iterator it) {
        return global_iterator(
            _glob_mem,
            _parent._graph->_myid,
            it.pos(),
            _glob_mem->container_size(_parent._graph->_myid, it.pos())
        );
      }

    private:

      parent_type &    _parent;
      glob_mem_type *  _glob_mem;

  };

  typedef edge_range<self_t, typename graph_type::glob_mem_edge_type> 
    inout_edge_range_type;
  typedef edge_range<self_t, typename graph_type::glob_mem_edge_comb_type> 
    edge_range_type;

public:

  VertexProxy() = delete;

  VertexProxy(iterator_type it, graph_type * g) 
    : _iterator(it),
      _graph(g),
      _out_edges(*this, g->_glob_mem_out_edge),
      _in_edges(*this, g->_glob_mem_in_edge),
      _edges(*this, g->_glob_mem_edge)
  { }

  inout_edge_range_type & out_edges() {
    return _out_edges;
  }

  inout_edge_range_type & in_edges() {
    return _in_edges;
  }

  edge_range_type & edges() {
    return _edges;
  }

  properties_type & properties() {
    // load properties lazily
    if(!_vertex_loaded) {
      _vertex = *_iterator;
      _vertex_loaded = true;
    }
    return _vertex.properties;
  }

private:

  iterator_type           _iterator;
  vertex_type             _vertex;
  bool                    _vertex_loaded = false;
  graph_type *            _graph;
  inout_edge_range_type   _out_edges;
  inout_edge_range_type   _in_edges;
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

  friend GraphType;
  
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
    : _source(owner, source.pos()),
      _target(owner, target.pos()),
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
    : _source(owner, source.pos()),
      _target(target.lpos().unit, target.lpos().index),
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
    : _source(source),
      _target(target),
      properties(properties)
  { }

public:

  /** Properties of this edge */
  properties_type       properties;

private:

  //TODO: Examine, if saving source can be avoided
  /** Source vertex the edge is pointing from */
  vertex_index_type     _source;
  /** Target vertex the edge is pointing to */
  vertex_index_type     _target;

};

template<typename GraphType, typename IteratorType>
class EdgeProxy {

  typedef GraphType                                         graph_type;
  typedef IteratorType                                      iterator_type;
  typedef typename GraphType::edge_type                     edge_type;
  typedef typename GraphType::edge_properties_type          properties_type;

public:

  EdgeProxy() = delete;

  EdgeProxy(iterator_type it, graph_type * g) 
    : _iterator(it),
      _graph(g)
  { }

  properties_type & properties() {
    // load properties lazily
    if(!_edge_loaded) {
      _edge = *_iterator;
      _edge_loaded = true;
    }
    return _edge.properties;
  }

private:

  iterator_type         _iterator;
  edge_type             _edge;
  bool                  _edge_loaded = false;
  graph_type *          _graph;

};

/**
 * Default property type holding no data.
 */
struct EmptyProperties { };

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
