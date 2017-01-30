#ifndef DASH__EDGE_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED
#define  DASH__EDGE_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED
namespace dash {

namespace graph {

/**
 * Pattern for graph data. Partitions the (logical) adjecency matrix of the 
 * graph into 2-dimensional blocks. Assigns one block to each unit.
 * NOTE: The number of units in the assigned team must be a power of 2.
 * NOTE: This pattern is intended for static graphs only.
 * 
 * \concept{DashDynamicPatternConcept}
 */
template<
  typename IndexType = std::size_t>
class EdgePartitionedDynamicPattern {

  typedef IndexType                                      index_type;
  typedef typename std::make_unsigned<IndexType>::type   size_type;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } local_index_type;

  /**
   * Constructs the partitioning scheme.
   */
  EdgePartitionedDynamicPattern(Team & team);

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

#endif // DASH__EDGE_PARTITIONED_DYNAMIC_PATTERN_H__INCLUDED
