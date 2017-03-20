#ifndef DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
#define DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED

namespace dash {

enum GraphDirection {
  UndirectedGraph,
  DirectedGraph
  //BidirectionalGraph
};

namespace internal {

template<
  typename IndexType,
  typename EdgeContainer,
  typename VertexProperties>
struct vertex {
  
  /**
   * Creates a vertex with given properties.
   */
  vertex(VertexProperties properties = VertexProperties()) 
    : _properties(properties)
  { }

  /** Outgoing edges from this vertex */
  EdgeContainer     _out_edges;
  /** Properties of this vertex */
  VertexProperties  _properties;
  /** Index of the vertex in local index space */
  IndexType         _local_id;

};

template<
  typename VertexIndexType,
  typename EdgeProperties>
struct out_edge {
  
  /**
   * Creates an edge from its parent vertex to target.
   */
  out_edge(VertexIndexType target, EdgeProperties properties = EdgeProperties()) 
    : _target(target),
      _properties(properties)
  { }

  /** Target vertex the edge is pointing to */
  VertexIndexType      _target;
  /** Properties of this edge */
  EdgeProperties  _properties;

};

struct EmptyProperties { };

}

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
