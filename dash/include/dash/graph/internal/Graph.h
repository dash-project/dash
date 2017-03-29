#ifndef DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
#define DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED

namespace dash {

enum GraphDirection {
  UndirectedGraph,
  DirectedGraph
  //BidirectionalGraph
};

namespace internal {

template <typename IndexType>
struct VertexIndex {

  VertexIndex() = default;

  VertexIndex(team_unit_t u, IndexType o) 
    : unit(u),
      offset(o)
  { }

  team_unit_t     unit;
  IndexType       offset;

};

template <typename IndexType>
struct EdgeIndex {

  EdgeIndex() = default;

  EdgeIndex(team_unit_t u, IndexType o) 
    : unit(u),
      offset(o)
  { }

  team_unit_t     unit;
  IndexType       offset;

};

template<typename GraphType>
struct vertex {

  typedef typename GraphType::edge_cont_ref_type       edge_container_ref;
  typedef typename GraphType::vertex_offset_type       index_type;
  typedef typename GraphType::vertex_properties_type   properties_type;

  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op
   */
  vertex() = default;
  
  /**
   * Creates a vertex with given properties.
   */
  vertex(
      index_type & index, 
      edge_container_ref & edge_ref, 
      properties_type properties = properties_type()
  ) 
    : _local_id(index),
      _edge_ref(edge_ref),
      _properties(properties)
  { }

  /** Properties of this vertex */
  properties_type       _properties;
  /** Index of the vertex in local index space */
  index_type            _local_id;
  /* 
   * Iterator to the position of the respective edge container in glob_mem 
   * object
   */
  edge_container_ref    _edge_ref;

};

template<typename GraphType>
struct out_edge {

  typedef typename GraphType::vertex_index_type        vertex_index_type;
  typedef typename GraphType::edge_offset_type         index_type;
  typedef typename GraphType::edge_properties_type     properties_type;
  
  /**
   * Default constructor needed for GlobRef / GlobSharedRef dereference op
   */
  out_edge() = default;

  /**
   * Creates an edge from its parent vertex to target.
   */
  out_edge(
      index_type index,
      vertex_index_type source,
      vertex_index_type target, 
      properties_type properties = properties_type()
  ) 
    : _local_id(index),
      _source(source),
      _target(target),
      _properties(properties)
  { }

  //TODO: Examine, if saving source can be avoided
  /** Source vertex the edge is pointing from */
  vertex_index_type     _source;
  /** Target vertex the edge is pointing to */
  vertex_index_type     _target;
  /** Properties of this edge */
  properties_type       _properties;
  /** Index of the edge in local index space */
  index_type            _local_id;

};

struct EmptyProperties { };

}

}

#endif // DASH__GRAPH__INTERNAL__GRAPH_H__INCLUDED
