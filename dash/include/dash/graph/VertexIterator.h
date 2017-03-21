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
  typedef typename Graph::global_vertex_iterator   iterator;
  typedef const iterator                           const_iterator;
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
    return _graph->_glob_mem_vertex->begin();
  }
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  const_iterator begin() const {
    return _graph->_glob_mem_vertex->begin();
  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  iterator end() {
    return _graph->_glob_mem_vertex->end();
  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  const_iterator end() const {
    return _graph->_glob_mem_vertex->end();
  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  local_iterator lbegin() {
    return _graph->_glob_mem_vertex->lbegin();
  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_local_iterator lbegin() const {
     return _graph->_glob_mem_vertex->lbegin();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  local_iterator lend() {
    return _graph->_glob_mem_vertex->lend();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_local_iterator lend() const {
    return _graph->_glob_mem_vertex->lend();
  }

private:
   
  graph_type *     _graph;

};

}

#endif // DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
