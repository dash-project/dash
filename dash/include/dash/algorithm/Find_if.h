#ifndef DASH__ALGORITHM__FIND_IF_H__
#define DASH__ALGORITHM__FIND_IF_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/**
 * \ingroup     DashAlgorithms
 */
template<
    typename ElementType,
    class PatternType,
	class UnaryPredicate>
GlobIter<ElementType, PatternType> find_if(
    /// Iterator to the initial position in the sequence
    GlobIter<ElementType, PatternType>   first,
    /// Iterator to the final position in the sequence
    GlobIter<ElementType, PatternType>   last,
    /// Predicate which will be applied to the elements in range [first, last)
	UnaryPredicate predicate)
{
  typedef dash::default_index_t index_t;

  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first, last);
  auto l_first       = index_range.begin;
  auto l_last        = index_range.end;

  auto l_result      = std::find_if(l_first, l_last, predicate);
  auto l_offset      = std::distance(l_first, l_result);
  if (l_result == l_last) {
    l_offset = -1;
  }

  dash::Array<index_t> l_results(dash::size());

  l_results.local[0] = l_offset;

  dash::barrier();

  // All local offsets stored in l_results

  for (auto u = 0; u < dash::size(); u++) {
    if (static_cast<index_x>(l_results[u]) >= 0) {
      auto g_offset = first.pattern()
                           .global_index(
                              u,
                              static_cast<index_t>(
                                l_results[u]));
      return first + g_offset - first.pos();
    }
  }

  return last;
}

} // namespace dash

#endif // DASH__ALGORITHM__FIND__IF_H__
