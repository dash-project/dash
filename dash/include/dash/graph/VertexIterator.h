#ifndef DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
#define DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED

namespace dash {

/**
 * Wrapper for the vertex iterators of the graph.
 */
template<typename Graph>
struct VertexIteratorWrapper {

  typedef Graph                                         graph_type;
  typedef typename graph_type::glob_mem_vert_type       glob_mem_type;
  typedef typename Graph::global_vertex_iterator        iterator;
  typedef const iterator                                const_iterator;
  typedef typename Graph::local_vertex_iterator         local_iterator;
  typedef const local_iterator                          const_local_iterator;
  typedef typename graph_type::vertex_properties_type   properties_type;
  typedef typename graph_type::vertex_size_type         size_type;

  /**
   * Constructs the wrapper.
   */
  VertexIteratorWrapper(graph_type * graph)
    : _graph(graph),
      _gmem(graph->_glob_mem_vertex)
  { }

  /*
   * Default constructor.
   */
  VertexIteratorWrapper() = default;

  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  iterator begin() {
    return _gmem->begin();
  }
  
  /**
   * Returns global iterator to the beginning of the vertex list.
   */
  const_iterator begin() const {
    return _gmem->begin();
  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  iterator end() {
    return _gmem->end();
  }
  
  /**
   * Returns global iterator to the end of the vertex list.
   */
  const_iterator end() const {
    return _gmem->end();
  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  local_iterator lbegin() {
    return _gmem->lbegin();
  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_local_iterator lbegin() const {
     return _gmem->lbegin();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  local_iterator lend() {
    return _gmem->lend();
  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_local_iterator lend() const {
    return _gmem->lend();
  }


  /**
   * Directly gets the attributes of a vertex identified by a local offset
   */
  properties_type & attributes(size_type local_index) {
    // TODO: Allow this only when there are no uncommitted changes
    return _gmem->get(local_index).properties;
  }

  /**
   * Directly sets the attributes of a vertex identified by a local offset
   */
  void set_attributes(size_type local_index, properties_type prop) {
    // TODO: Allow this only when there are no uncommitted changes
    auto & v = _gmem->get(local_index);
    v.properties = prop;
  }
  
  /**
   * Returns the amount of vertices in the whole graph.
   */
  size_type size() {
    return _gmem->size();
  }

  /**
   * Returns the amount of vertices the specified unit currently holds in 
   * global memory space.
   */
  size_type size(team_unit_t unit) {
    return _gmem->size(unit);
  }

  /**
   * Returns the amount of vertices this unit currently holds in local memory
   * space.
   */
  size_type lsize() {
    return _gmem->lsize();
  }

  /*
   * Returns whether theare are vertices in global memory space.
   */
  bool empty() {
    return size() == 0;
  }

  /**
   * Returns the maximum number of vertices the graph can store.
   */
  size_type max_size() {
    return std::numeric_limits<size_type>::max();
  }

private:
   
  graph_type *     _graph;
  glob_mem_type *  _gmem;

};

}

#endif // DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
