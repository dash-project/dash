#ifndef DASH__ALGORITHM__IS_PARTITIONED_H__
#define DASH__ALGORITHM__IS_PARTITIONED_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {


/**
 * Returns true if all elements in the range [first, last) that 
 * statisfy the predicate p appear before all elements that don't. Also returns
 * true if [first, last) is empty.
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
  class    UnaryPredicate>
bool is_partitioned(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// UnaryPredicate to check with elements of sequence
  const ElementType                  & value)
{
  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first, last);
  auto l_first       = index_range.begin;
  auto l_last        = index_range.end;

  auto l_result      = std::is_partitioned(l_first, l_last, value);

  dash::Array<dart_unit_t> l_results(dash::size());

  l_results.local[0] = l_result;

  dash::barrier();

  // All local offsets stored in l_results
  for (auto u = 0; u < dash::size(); u++) {
    if (l_results[u] != true) {
      return false;
    }
  }
  return true;
}
}// namespace dash

#endif // DASH__ALGORITHM__IS_PARTITIONED_H__
