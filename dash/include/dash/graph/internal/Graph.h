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
 * Index type for edges.
 */
template <typename IndexType>
struct EdgeIndex {

  /**
   * Default constructor.
   */
  EdgeIndex() = default;

  /**
   * Index Constructor.
   */
  EdgeIndex(team_unit_t u, IndexType c, IndexType o) 
    : unit(u),
      container(c),
      offset(o)
  { }

  /** The unit holding the referenced edge */
  team_unit_t     unit;
  /** The container in globale memory space of unit containing the edge */
  IndexType       container;
  /** The offset in the referenced container */
  IndexType       offset;

};

/**
 * Vertex type holding properties and references to its corresponding edge 
 * lists.
 */
template<typename GraphType>
class Vertex {

  typedef typename GraphType::edge_cont_ref_type       edge_container_ref;
  typedef typename GraphType::vertex_index_type        index_type;
  typedef typename GraphType::vertex_properties_type   properties_type;

  friend GraphType;
  friend InEdgeIteratorWrapper<GraphType>;
  friend OutEdgeIteratorWrapper<GraphType>;
  friend EdgeIteratorWrapper<GraphType>;

public:

  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op.
   */
  Vertex() = default;
  
  /**
   * Creates a vertex with given properties.
   */
  Vertex(
      const index_type & index, 
      const edge_container_ref & in_edge_ref, 
      const edge_container_ref & out_edge_ref, 
      const properties_type & properties
  ) 
    : _index(index),
      _in_edge_ref(in_edge_ref),
      _out_edge_ref(out_edge_ref),
      properties(properties)
  { }

public:

  /** Properties of this vertex */
  properties_type       properties;

private:

  /** Index of the vertex in local index space */
  index_type            _index;
  /** index of the in-edge list belonging to this vertex */
  edge_container_ref    _in_edge_ref;
  /** index of the out-edge list belonging to this vertex */
  edge_container_ref    _out_edge_ref;

};

/**
 * Edge type holding properties and references to the vertices it belongs to.
 */
template<typename GraphType>
class Edge {

  typedef typename GraphType::vertex_index_type        vertex_index_type;
  typedef typename GraphType::edge_index_type          index_type;
  typedef typename GraphType::edge_properties_type     properties_type;

  friend GraphType;
  
public:

  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op.
   */
  Edge() = default;

  /**
   * Creates an edge with index.
   */
  Edge(
      const index_type & index,
      const vertex_index_type & source,
      const vertex_index_type & target, 
      const properties_type & properties
  ) 
    : _index(index),
      _source(source),
      _target(target),
      properties(properties)
  { }

  /**
   * Creates an edge without index. Needed for edges that belong to other 
   * units.
   */
  Edge(
      const vertex_index_type & source,
      const vertex_index_type & target, 
      const properties_type & properties
  ) 
    : _index(),
      _source(source),
      _target(target),
      properties(properties)
  { }

public:

  /** Properties of this edge */
  properties_type       properties;

  /**
   * Returns the source vertex of the edge
   */
  vertex_index_type source() const {
    return _source;
  }

  /**
   * Returns the target vertex of the edge
   */
  vertex_index_type target() const {
    return _target;
  }

private:

  //TODO: Examine, if saving source can be avoided
  /** Source vertex the edge is pointing from */
  vertex_index_type     _source;
  /** Target vertex the edge is pointing to */
  vertex_index_type     _target;
  /** Index of the edge in local index space */
  index_type            _index;

};

/**
 * Default property type holding no data.
 */
struct EmptyProperties { };

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
