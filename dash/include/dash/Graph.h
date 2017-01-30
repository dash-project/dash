#ifndef DASH__GRAPH_H__INCLUDED
#define DASH__GRAPH_H__INCLUDED

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

/**
 * Wrapper for the vertex iterators of the graph.
 */
struct VertexIteratorWrapper {

  typedef VertexIterator             iterator;
  typedef const VertexIterator       const_iterator;
  typedef LocalVertexIterator        local_iterator;
  typedef const LocalVertexIterator  const_local_iterator;
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  iterator begin();
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  const_iterator begin() const;
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  iterator end();
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  const_iterator end() const;
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  local_iterator lbegin();
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_local_iterator lbegin() const;
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  local_iterator lend();
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_local_iterator lend() const;

};

// TODO: add all iterator types + wrappers

}

/**
 * Distributed, dynamic graph container for sparse graphs.
 */
template<
  GraphDirection Direction  = dash::graph::DirectedGraph,
  typename DynamicPattern   = dash::graph::VertexPartitionedDynamicPattern,
  typename VertexProperties = Empty,           // user-defined struct
  typename EdgeProperties   = Empty,           // user-defined struct
  typename VertexContainer  = std::vector,
  typename EdgeContainer    = std::vector,
  typename VertexSizeType   = std::size_t,
  typename EdgeSizeType     = std::size_t>
class Graph {

  typedef detail::VertexIteratorWrapper               vertex_it_wrapper;
  typedef detail::EdgeIteratorWrapper                 edge_it_wrapper;
  typedef detail::InEdgeIteratorWrapper               in_edge_it_wrapper;
  typedef detail::OutEdgeIteratorWrapper              out_edge_it_wrapper;
  typedef detail::AdjacencyIteratorWrapper            adjacency_it_wrapper;

public:

  typedef typename detail::vertex::index_type         vertex_index_type;
  typedef typename detail::edge<Direction, 
          vertex_index_type>::index_type              edge_index_type;
  typedef VertexSizeType                              vertex_size_type;
  typedef EdgeSizeType                                edge_size_type;

  typedef Direction                                   direction_type;
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

  vertex_it_wrapper     vertices;
  edge_it_wrapper       edges;
  in_edge_it_wrapper    in_edges;
  out_edge_it_wrapper   out_edges;
  adjacency_it_wrapper  adjacent_vertices;

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
  VertexProperties & operator[](const vertex_index_type & v) const;

  /**
   * Returns a property object for the given edge.
   */
  EdgeProperties & operator[](const edge_index_type & e) const;

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

#endif // DASH__GRAPH_H__INCLUDED

