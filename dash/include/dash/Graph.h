#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

namespace dash {

/**
 * \defgroup DashGraphConcept Graph Concept
 * Concept of a statically distributed graph container.
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

namespace graph {

enum GraphDirection {
  DirectedGraph,
  BidirectionalGraph,
  UndirectedGraph
};

}

namespace detail {

class vertex {
  // TODO: define vertex interface
}

template<
  GraphDirection Direction,
  typename VertexDescriptor>
class edge {
  // TODO: define edge interface
};

// TODO: find template paramters
class VertexIterator {

}

struct VertexIteratorWrapper {

  typedef VertexIterator vertex_iterator;
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  vertex_iterator begin();
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  const_vertex_iterator begin() const;
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  vertex_iterator end();
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  const_vertex_iterator end() const;
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  vertex_iterator lbegin();
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_vertex_iterator lbegin() const;
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  vertex_iterator lend();
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_vertex_iterator lend() const;

};

// TODO: add all iterator types + wrappers

}

template<
  GraphDirection Direction  = dash::graph::DirectedGraph,
  typename DynamicPattern   = dash::graph::VertexPartitionedDynamicPattern,
  typename VertexProperties = Empty,           // user-defined struct
  typename EdgeProperties   = Empty,           // user-defined struct
  typename GraphProperties  = Empty,           // user-defined struct
  typename VertexContainer  = std::vector,
  typename EdgeContainer    = std::vector,
  typename VertexSizeType   = std::size_t,
  typename EdgeSizeType     = std::size_t>
class Graph {

  typedef detail::VertexIteratorWrapper               vertex_it_wrapper;

public:

  typedef detail::vertex                              vertex_descriptor;
  typedef detail::edge<Direction, vertex_descriptor>  edge_descriptor;
  typedef VertexSizeType                              vertex_size_type;
  typedef vertex_size_type                            vertex_index_type;
  typedef EdgeSizeType                                edge_size_type;
  typedef edge_size_type                              edge_index_type;

  typedef Direction                                   direction_type;
  typedef DynamicPattern                              pattern_type;
  typedef VertexContainer                             vertex_container_type;
  typedef EdgeContainer                               edge_container_type;
  typedef VertexProperties                            vertex_properties_type;
  typedef EdgeProperties                              edge_properties_type;

  typedef vertex_it_wrapper::vertex_iterator          vertex_iterator;
  typedef                                             edge_iterator;
  typedef                                             in_edge_iterator;
  typedef                                             out_edge_iterator;
  typedef                                             adjacency_iterator;

public:

  vertex_it_wrapper vertices;

public:

  /**
   * Constructs a graph with given properties
   * NOTE: GraphProperties() probably won't work, if the template argument 
   *       is empty
   */
  Graph(const GraphProperties & prop = GraphProperties());

  /**
   * Copy-constructs graph from another one.
   */
  Graph(const Graph & other);

  /**
   * Copy-assigns data from another graph.
   */
  Graph & operator=(const Graph & other);

  /**
   * Returns a property object for the given vertex.
   */
  VertexProperties & operator[](const vertex_descriptor & v) const;

  /**
   * Returns a property object for the given edge.
   */
  EdgeProperties & operator[](const edge_descriptor & e) const;

  /**
   * TODO: Return full property object for graph properties or
   *       use getter- and setter methods for individual fields?
   *       Also: Are graph properties really needed? The user could just
   *       save them elsewhere.
   */
  
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
  vertex_descriptor add_vertex();

  /**
   * Adds a vertex with the given properties.
   * NOTE: global method.
   */
  vertex_descriptor add_vertex(const VertexProperties & prop);

  /**
   * Removes a given vertex.
   * NOTE: global method.
   */
  void remove_vertex(const vertex_descriptor & v);

  /**
   * Removes all edges (in & out) from the given vertex).
   * NOTE: global method.
   */
  void clear_vertex(vertex_descriptor & v);

  /**
   * Adds an edge between two given vertices with default properties.
   * NOTE: global method.
   * 
   * \return pair, with pair::first set to the edge_descriptor of the added 
   * edge 
   */
  std::pair<edge_descriptor, bool> add_edge(const vertex_descriptor & v1, 
      const vertex_descriptor & v2);

  /**
   * Adds an edge between two given vertices with the given properties 
   * locally.
   * NOTE: global method.
   */
  std::pair<edge_descriptor, bool> add_edge(const vertex_descriptor & v1, 
      const vertex_descriptor & v2, const EdgeProperties & prop);

  /**
   * Removes an edge between two given vertices.
   * NOTE: global method.
   */
  void remove_edge(const vertex_descriptor & v1, 
      const vertex_descriptor & v2);

  /**
   * Removes a given edge.
   * NOTE: global method.
   */
  void remove_edge(const edge_descriptor & e);

  /**
   * Commits local changes performed by methods classified as "global" to
   * the whole data structure.
   */
  void barrier();

};

}

#endif // DASH__GRAPH_H__INCLUDED

