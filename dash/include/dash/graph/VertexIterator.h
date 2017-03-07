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
template<
  typename VertexIndexType,
  typename VertexProperties>
struct VertexIteratorWrapper {

  typedef VertexIterator             iterator;
  typedef const VertexIterator       const_iterator;
  typedef LocalVertexIterator        local_iterator;
  typedef const LocalVertexIterator  const_local_iterator;
  typedef VertexIndexType            vertex_index_type;
  typedef VertexProperties           vertex_properties_type;

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

  }
  
  /**
   * Returns local iterator to the beginning of the vertex list.
   */
  const_local_iterator lbegin() const {

  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  local_iterator lend() {

  }
  
  /**
   * Returns local iterator to the end of the vertex list.
   */
  const_local_iterator lend() const {

  }

};

}

#endif // DASH__GRAPH__VERTEX_ITERATOR_H__INCLUDED
