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
      const properties_type & properties
  ) 
    : properties(properties)
  { }

public:

  /** Properties of this vertex */
  properties_type       properties;

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

/**
 * Default property type holding no data.
 */
struct EmptyProperties { };

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
