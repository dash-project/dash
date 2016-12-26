#ifndef DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED
#define DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED

#include <dash/iterator/GlobIter.h>
#include <dash/internal/Logging.h>

#include <dash/algorithm/LocalRange.h>


namespace dash {

template<typename IndexType>
struct LocalIndexRanges {
  std::vector< dash::LocalIndexRange<IndexType> > ranges;
};


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
}

} // namespace dash

#endif // DASH__ALGORITHM__LOCAL_RANGES_H__INCLUDED
