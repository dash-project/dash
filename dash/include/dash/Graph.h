#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

#include <vector>
#include <dash/graph/VertexIterator.h>
#include <dash/graph/internal/Graph.h>

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
  typename VertexContainer  = std::vector<internal::vertex>,
  typename EdgeContainer    = std::vector<internal::vertex>,
  typename VertexIndexType  = int,
  typename EdgeIndexType    = int>
class Graph {

  // TODO: add wrapper for all iterator types
  typedef VertexIteratorWrapper 
    <VertexIndexType, VertexProperties>               vertex_it_wrapper;
  typedef VertexIteratorWrapper  
    <VertexIndexType, VertexProperties>               edge_it_wrapper;
  typedef VertexIteratorWrapper 
    <VertexIndexType, VertexProperties>               in_edge_it_wrapper;
  typedef VertexIteratorWrapper  
    <VertexIndexType, VertexProperties>               out_edge_it_wrapper;
  typedef VertexIteratorWrapper  
    <VertexIndexType, VertexProperties>               adjacency_it_wrapper;

public:

  typedef Graph<Direction, DynamicPattern, 
          VertexProperties, EdgeProperties,
          VertexContainer, EdgeContainer, 
          VertexIndexType, EdgeIndexType>             graph_type;
  typedef VertexIndexType                             vertex_index_type;
  typedef EdgeIndexType                               edge_index_type;
  typedef typename 
    std::make_unsigned<VertexIndexType>::type         vertex_size_type;
  typedef typename 
    std::make_unsigned<EdgeIndexType>::type           edge_size_type;

  typedef DynamicPattern                              pattern_type;
  typedef VertexContainer                             vertex_container_type;
  typedef EdgeContainer                               edge_container_type;
  typedef VertexProperties                            vertex_properties_type;
  typedef EdgeProperties                              edge_properties_type;

  typedef typename vertex_it_wrapper::iterator        vertex_iterator;
  typedef typename edge_it_wrapper::iterator          edge_iterator;
  typedef typename in_edge_it_wrapper::iterator       in_edge_iterator;
  typedef typename out_edge_it_wrapper::iterator      out_edge_iterator;
  typedef typename adjacency_it_wrapper::iterator     adjacency_iterator;

public:

  vertex_it_wrapper      vertices;
  edge_it_wrapper        edges;
  in_edge_it_wrapper     in_edges;
  out_edge_it_wrapper    out_edges;
  adjacency_it_wrapper   adjacent_vertices;

public:

  /**
   * Constructs an empty graph.
   */
  Graph();
  
  /**
   * Copy-constructs graph from another one.
   */
  Graph(const graph_type & other);

  /**
   * Copy-assigns data from another graph.
   */
  graph_type & operator=(const graph_type & other);

  /**
   * Returns the number of vertices in the whole graph.
   */
  const vertex_size_type num_vertices() const;

  /**
   * Returns the index of the vertex with the highest index in the whole graph.
   */
  const vertex_index_type max_vertex_index() const;

  /**
   * Returns the number of edges in the whole graph.
   */
  const edge_size_type num_edges() const;

  /**
   * Returns the index of the edge with the highest index in the whole graph.
   */
  const edge_index_type max_edge_index() const;

  /**
   * Returns, whether the graph is empty.
   */
  bool empty() const;

  /**
   * Adds a vertex with default properties.
   * NOTE: global method.
   */
  vertex_index_type add_vertex();

  /**
   * Adds a vertex with the given properties.
   * NOTE: global method.
   */
  vertex_index_type add_vertex(const VertexProperties & prop);

  /**
   * Removes a given vertex.
   * NOTE: global method.
   */
  void remove_vertex(const vertex_index_type & v);

  /**
   * Removes all edges (in & out) from the given vertex).
   * NOTE: global method.
   */
  void clear_vertex(vertex_index_type & v);

  /**
   * Adds an edge between two given vertices with default properties.
   * NOTE: global method.
   * 
   * \return pair, with pair::first set to the edge_descriptor of the added 
   * edge 
   */
  std::pair<edge_index_type, bool> add_edge(const vertex_index_type & v1, 
      const vertex_index_type & v2);

  /**
   * Adds an edge between two given vertices with the given properties 
   * locally.
   * NOTE: global method.
   */
  std::pair<edge_index_type, bool> add_edge(const vertex_index_type & v1, 
      const vertex_index_type & v2, const EdgeProperties & prop);

  /**
   * Removes an edge between two given vertices.
   * NOTE: global method.
   */
  void remove_edge(const vertex_index_type & v1, 
      const vertex_index_type & v2);

  /**
   * Removes a given edge.a9f5c03d9de8dda4
   * NOTE: global method.
   */
  void remove_edge(const edge_index_type & e);

  /**
   * Commits local changes performed by methods classified as "global" to
   * the whole data structure.
   */
  void barrier();
};

}

#include <dash/graph/Graph-impl.h>

#endif // DASH__GRAPH_H__INCLUDED

