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

}

template<
  GraphDirection Direction  = dash::graph::DirectedGraph,
  typename Partition        = dash::graph::VertexPartition,
  typename VertexProperties = Empty,           // user-defined struct
  typename EdgeProperties   = Empty,           // user-defined struct
  typename GraphProperties  = Empty,           // user-defined struct
  typename VertexContainer  = std::vector,
  typename EdgeContainer    = std::vector,
  typename VertexSizeType   = std::size_t,
  typename EdgeSizeType     = std::size_t>
class Graph {

public:

  typedef detail::vertex                              vertex_descriptor;
  typedef detail::edge<Direction, vertex_descriptor>  edge_descriptor;
  typedef VertexSizeType                              vertex_size_type;
  typedef vertex_size_type                            vertex_index_type;
  typedef EdgeSizeType                                edge_size_type;
  typedef edge_size_type                              edge_index_type;

  typedef Direction                                   direction_type;
  typedef Partition                                   partition_type;
  typedef VertexContainer                             vertex_container_type;
  typedef EdgeContainer                               edge_container_type;
  typedef VertexProperties                            vertex_properties_type;
  typedef EdgeProperties                              edge_properties_type;

  // iterators:
  // TODO: Find types
  //       Things to consider:
  //       - Undifrected Graph: no difference betw. in- and out-edge 
  //         iterators and edge_iterator
  typedef   vertex_iterator;
  typedef   edge_iterator;
  typedef   in_edge_iterator;
  typedef   out_edge_iterator;
  typedef   adjacency_iterator;

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
   *       Also: Are graph properties rellay needed? The user could just
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
  void commit();

  /**
   * Returns iterator to the beginning of the vertex list.
   */
  vertex_iterator v_begin();

  /**
   * TODO: Iterator method names are not really straightforward.
   */
  vertex_iterator v_end();
  edge_iterator e_begin();
  edge_iterator e_end();
  in_edge_iterator ie_begin();
  in_edge_iterator ie_end();
  out_edge_iterator oe_begin();
  out_edge_iterator oe_end();
  adjacency_iterator a_begin();
  adjacency_iterator a_end();

  vertex_iterator v_lbegin();
  vertex_iterator v_lend();
  edge_iterator e_lbegin();
  edge_iterator e_lend();
  in_edge_iterator ie_lbegin();
  in_edge_iterator ie_lend();
  out_edge_iterator oe_lbegin();
  out_edge_iterator oe_lend();
  adjacency_iterator a_lbegin();
  adjacency_iterator a_lend();

};

}

#endif // DASH__GRAPH_H__INCLUDED

