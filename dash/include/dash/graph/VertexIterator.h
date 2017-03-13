#ifndef DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
#define DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED

namespace dash {

class VertexIterator {

};

class LocalVertexIterator {

};

/**
 * Wrapper for the vertex iterators of the graph.
 */
template<typename Graph>
struct VertexIteratorWrapper {

  typedef Graph                                    graph_type;
  typedef VertexIterator                           iterator;
  typedef const VertexIterator                     const_iterator;
  typedef typename Graph::local_vertex_iterator    local_iterator;
  typedef const local_iterator                     const_local_iterator;
  typedef typename Graph::vertex_index_type        vertex_index_type;
  typedef typename Graph::vertex_properties_type   vertex_properties_type;

  /**
   * Constructs the wrapper.
   */
  VertexIteratorWrapper(graph_type * graph) 
    : _graph(graph)
  { }

 /**
   * Returns a property object for the given vertex.
   */
  vertex_properties_type & operator[](const vertex_index_type & v) const {

  }

  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  iterator begin() {

  }
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  const_iterator begin() const {

  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  iterator end() {

  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  const_iterator end() const {

  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  local_iterator lbegin() {
    return _graph->_glob_mem_con->lbegin();
  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_local_iterator lbegin() const {
     return _graph->_glob_mem_con->lbegin();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  local_iterator lend() {
    return _graph->_glob_mem_con->lend();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_local_iterator lend() const {
    return _graph->_glob_mem_con->lend();
  }

private:
   
  graph_type *     _graph;

};

}

#endif // DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
