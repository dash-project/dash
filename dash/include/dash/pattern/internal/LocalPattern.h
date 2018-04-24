#ifndef DASH__PATTERN__INTERNAL__LOCAL_PATTERN_H__INCLUDED
#define DASH__PATTERN__INTERNAL__LOCAL_PATTERN_H__INCLUDED

#include <dash/Types.h>

#include <dash/pattern/PatternProperties.h>


namespace dash {
namespace internal {

template<
  dim_t      NumDimensions,
  MemArrange Arrangement      = dash::ROW_MAJOR,
  typename   IndexType        = dash::default_index_t >
class LocalPattern;



template<
  MemArrange Arrangement,
  typename   IndexType >
class LocalPattern<1, Arrangement, IndexType> {
private:
  static const dim_t NumDimensions = 1;

public:
  static constexpr char const * PatternName = "LocalPattern";

public:
  /// Satisfiable properties in pattern property category Partitioning:
  typedef pattern_partitioning_properties<
              // Block extents are constant for every dimension.
              pattern_partitioning_tag::rectangular,
              // Identical number of elements in every block.
              pattern_partitioning_tag::balanced,
              // Size of blocks may differ.
              pattern_partitioning_tag::unbalanced
          > partitioning_properties;
  /// Satisfiable properties in pattern property category Mapping:
  typedef pattern_mapping_properties<
              // Number of blocks assigned to a unit may differ.
              pattern_mapping_tag::unbalanced
          > mapping_properties;
  /// Satisfiable properties in pattern property category Layout:
  typedef pattern_layout_properties<
              // Local indices iterate over block boundaries.
              pattern_layout_tag::canonical,
              // Local element order corresponds to canonical linearization
              // within entire local memory.
              pattern_layout_tag::linear
          > layout_properties;
};

} // namespace internal
} // namespace dash

#endif // DASH__PATTERN__INTERNAL__LOCAL_PATTERN_H__INCLUDED
