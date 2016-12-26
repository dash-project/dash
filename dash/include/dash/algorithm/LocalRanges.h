#ifndef DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED
#define DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED

#include <dash/iterator/GlobIter.h>
#include <dash/pattern/PatternProperties.h>
#include <dash/internal/Logging.h>

#include <dash/algorithm/LocalRange.h>


namespace dash {

template<typename IndexType>
struct LocalIndexRanges {
  std::vector< dash::LocalIndexRange<IndexType> > ranges;
};


namespace internal {

/**
 * Pattern is tiled (block elements are contiguous)
 */
template<class GlobInputIter>
typename std::enable_if<
  ( !GlobInputIter::has_view::value &&
    dash::pattern_constraints<
      dash::pattern_partitioning_properties<>,
      dash::pattern_mapping_properties<>,
      dash::pattern_layout_properties<
        dash::pattern_layout_tag::blocked
      >,
      typename GlobInputIter::pattern_type
    >::satisfied::value ),
  LocalIndexRanges<typename GlobInputIter::pattern_type::index_type>
>::type
local_index_ranges_impl(
  /// Iterator to the initial position in the global sequence
  const GlobInputIter & first,
  /// Iterator to the final position in the global sequence
  const GlobInputIter & last)
{
  typedef typename GlobInputIter::pattern_type pattern_t;
  typedef typename pattern_t::index_type       idx_t;

  LocalIndexRanges<idx_t> res;

  return res;
}

/**
 * Pattern is strided (elements are not contiguous in single blocks)
 */
template<class GlobInputIter>
typename std::enable_if<
  ( !GlobInputIter::has_view::value &&
    !dash::pattern_constraints<
      dash::pattern_partitioning_properties<>,
      dash::pattern_mapping_properties<>,
      dash::pattern_layout_properties<
        dash::pattern_layout_tag::blocked
      >,
      typename GlobInputIter::pattern_type
    >::satisfied::value ),
  LocalIndexRanges<typename GlobInputIter::pattern_type::index_type>
>::type
local_index_ranges_impl(
  /// Iterator to the initial position in the global sequence
  const GlobInputIter & first,
  /// Iterator to the final position in the global sequence
  const GlobInputIter & last)
{
  typedef typename GlobInputIter::pattern_type pattern_t;
  typedef typename pattern_t::index_type       idx_t;

  LocalIndexRanges<idx_t> res;

  return res;
}

} // namespace internal


template<class GlobInputIter>
typename std::enable_if<
  !GlobInputIter::has_view::value,
  LocalIndexRanges<typename GlobInputIter::pattern_type::index_type>
>::type
local_index_ranges(
  /// Iterator to the initial position in the global sequence
  const GlobInputIter & first,
  /// Iterator to the final position in the global sequence
  const GlobInputIter & last)
{
  typedef typename GlobInputIter::pattern_type pattern_t;
  typedef typename pattern_t::index_type       idx_t;

  LocalIndexRanges<idx_t> res = internal::local_index_ranges_impl(
                                  first, last);

  return res;
}

} // namespace dash

#endif // DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED
