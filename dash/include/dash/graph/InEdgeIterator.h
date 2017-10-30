#ifndef DASH__GRAPH__IN_EDGE_ITERATOR_H__INCLUDED
#define DASH__GRAPH__IN_EDGE_ITERATOR_H__INCLUDED

namespace dash {

/**
 * Wrapper for the edge iterators of the graph.
 */
template<typename Graph>
struct InEdgeIteratorWrapper {

  typedef Graph                                       graph_type;
  typedef typename Graph::global_in_edge_iterator     iterator;
  typedef const iterator                              const_iterator;
  typedef typename Graph::local_in_edge_iterator      local_iterator;
  typedef const local_iterator                        const_local_iterator;
  typedef typename Graph::edge_properties_type        edge_properties_type;

  /**
   * Constructs the wrapper.
   */
  InEdgeIteratorWrapper(graph_type * graph)
    : _graph(graph)
  { }

  /**
   * Returns global iterator to the beginning of the edge list.
   */
  iterator begin() {
    return _graph->_glob_mem_in_edge->begin();
    //return iterator(_graph->_glob_mem_in_edge, 0);
  }
  
  /**
   * Returns global iterator to the beginning of the edge list.
   */
  const_iterator begin() const {
    return _graph->_glob_mem_in_edge->begin();
    //return iterator(_graph->_glob_mem_in_edge, 0);
  }
  
  /**
   * Returns global iterator to the end of the edge list.
   */
  iterator end() {
    return _graph->_glob_mem_in_edge->end();
    //return iterator(_graph->_glob_mem_in_edge, _graph->_glob_mem_in_edge->size());
  }
  
  /**
   * Returns global iterator to the end of the edge list.
   */
  const_iterator end() const {
    return _graph->_glob_mem_in_edge->end();
    //return iterator(_graph->_glob_mem_in_edge, _graph->_glob_mem_in_edge->size());
  }
  
  /**
   * Returns local iterator to the beginning of the edge list.
   */
  local_iterator lbegin() {
    return _graph->_glob_mem_in_edge->lbegin();
  }
  
  /**
   * Returns local iterator to the beginning of the edge list.
   */
  const_local_iterator lbegin() const {
     return _graph->_glob_mem_in_edge->lbegin();
  }
  
  /**
   * Returns local iterator to the end of the edge list.
   */
  local_iterator lend() {
    return _graph->_glob_mem_in_edge->lend();
  }
  
  /**
   * Returns local iterator to the end of the edge list.
   */
  const_local_iterator lend() const {
    return _graph->_glob_mem_in_edge->lend();
  }

private:
   
  graph_type *     _graph;

};

}

#endif // DASH__GRAPH__IN_EDGE_ITERATOR_H__INCLUDED
