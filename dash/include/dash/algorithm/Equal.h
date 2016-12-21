#ifndef DASH__ALGORITHM__EQUAL_H__
#define DASH__ALGORITHM__EQUAL_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/**
 * Returns true if the range \c [first1, last1) is equal to the range
 * \c [first2, first2 + (last1 - first1)), and false otherwise.
 *
 * \ingroup     DashAlgorithms
 */
template <
  typename ElementType,
  class    PatternType >
bool equal(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first_1,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last_1,
  GlobIter<ElementType, PatternType>   first_2)
{
  auto & team        = first_1.team();
  auto myid          = team.myid();
  // Global iterators to local range:
  auto index_range   = dash::local_range(first_1, last_1);
  auto l_first_1     = index_range.begin;
  auto l_last_1      = index_range.end;
  auto l_result      = std::equal(l_first_1, l_last_1, first_2);

  dash::Array<bool> l_results(team.size());

  l_results.local[0] = l_result;
  bool return_result = true;

  // wait for all units to contribute their data
  team.barrier();

  // All local offsets stored in l_results
  if (myid == 0) {
    for (int u = 0; u < team.size(); u++) {
      return_result &= l_results.local[u];
    }
  }
  return return_result;
}

/**
 * Returns true if the range \c [first1, last1) is equal to the range
 * \c [first2, first2 + (last1 - first1)) with respect to a specified
 * predicate, and false otherwise.
 *
 * \ingroup     DashAlgorithms
 */
template <
  typename ElementType,
  class    PatternType,
  class    BinaryPredicate >
bool equal(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first_1,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last_1,
  GlobIter<ElementType, PatternType>   first_2,
  BinaryPredicate                      pred)
{
  auto & team        = first_1.team();
  auto myid          = team.myid();
  // Global iterators to local range:
  auto index_range   = dash::local_range(first_1, last_1);
  auto l_first_1     = index_range.begin;
  auto l_last_1      = index_range.end;
  auto l_result      = std::equal(l_first_1, l_last_1, first_2, pred);

  dash::Array<bool> l_results(team.size());

  l_results.local[0] = l_result;

  bool return_result = true;

  team.barrier();

  // All local offsets stored in l_results
  if (myid == 0) {
    for (int u = 0; u < team.size(); u++) {
      return_result &= l_results.local[u];
    }
  }
  return return_result;
}

} // namespace dash

#endif // DASH__ALGORITHM__EQUAL_H__
