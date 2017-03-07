#ifndef DASH__VERTEX_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED
#define  DASH__VERTEX_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED

#include <type_traits>

namespace dash {

namespace graph {

namespace detail {

/**
 * Functor that computes the unit ID for a given vertex ID in a team.
 */
class VertexHashDistribution {

  /**
   * Constructs the Functor.
   */
  VertexHashDistribution(Team & team);
  
  /**
   * Maps a vertex ID to the corresponding unit.
   * TODO: argument should be Graph::size_type
   */
  dart_unit_t operator()(int id);

};

}


/**
 * Pattern for graph data. Partitions the data by vertex and 
 * distributes these vertices to units using some hash function.
 * 
 * \concept{DashDynamicPatternConcept}
 */
template<
  class DistributionMethod = dash::graph::detail::VertexHashDistribution,
  typename IndexType       = int>
class VertexPartitionedDynamicPattern {

  typedef IndexType                                      index_type;
  typedef typename std::make_unsigned<IndexType>::type   size_type;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } local_index_type;

  /**
   * Constructs the partitioning scheme.
   */
  VertexPartitionedDynamicPattern(Team & team);

  /**
   * Converts the given global vertex index to the owning unit.
   */
  dart_unit_t unit_at(index_type global_index) const;

  /**
   * Converts the given global vertex index to the owning unit and its 
   * local index.
   */
  local_index_type local(index_type global_index) const;

  /**
   * Converts the goiven local index of the calling unit to the respective
   * global index.
   */
  index_type global(index_type local_index) const;

  /**
   * Returns, whether the element at the given global index is locak to
   * the given unit.
   */
  bool is_local(index_type global_index, dart_unit_t unit) const;

  /**
   * Returns the amount of elements for which memory has been allocated.
   */
  size_type capacity() const;

  /**
   * Returns the amount of elements for which memory has been allocated at the 
   * calling unit.
   */
  size_type local_capacity() const;

  /**
   * Returns the amount of elements currently indexed in the pattern.
   */
  size_type size() const;

  /**
   * Returns the amount of elements currently indexed in the pattern local to
   * the calling unit.
   */
  size_type local_size() const;

};

}

}

#endif // DASH__VERTEX_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED
