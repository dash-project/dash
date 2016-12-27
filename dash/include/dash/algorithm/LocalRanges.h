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

  auto pattern = first.pattern();

  idx_t l_offset = 0;
  for (auto lblock_idx : pattern.local_blockspec()) {
    auto lblock_view = pattern.local_block_local(lblock_idx);
    auto lblock_size = lblock_view.size();

    LocalIndexRange<idx_t> lblock_irange;
    lblock_irange.begin = l_offset;
    lblock_irange.end   = l_offset + lblock_size;
    res.ranges.push_back(lblock_irange);

    l_offset += lblock_size;
  }

  return res;
}

/**
 * Pattern is strided (elements are not contiguous in single blocks)
 *
 * \todo   Not implemented
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


/**
 * \todo   Not implemented for view ranges, yet
 */
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
