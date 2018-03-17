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
template <typename GlobIter>
bool equal(
    /// Iterator to the initial position in the sequence
    GlobIter first_1,
    /// Iterator to the final position in the sequence
    GlobIter last_1,
    GlobIter first_2)
{
  static_assert(
      dash::iterator_traits<GlobIter>::is_global_iterator::value,
      "invalid iterator: Need to be a global iterator");

  auto & team        = first_1.team();
  auto myid          = team.myid();
  // Global iterators to local range:
  auto index_range   = dash::local_range(first_1, last_1);
  auto l_first_1     = index_range.begin;
  auto l_last_1      = index_range.end;
  char l_result      = static_cast<char>(std::equal(l_first_1, l_last_1, first_2));
  char r_result      = 0;

  DASH_ASSERT_RETURNS(
    dart_allreduce(&l_result, &r_result, 1,
      DART_TYPE_BYTE, DART_OP_BAND, team.dart_id()),
    DART_OK);
  return r_result;
}

/**
 * Returns true if the range \c [first1, last1) is equal to the range
 * \c [first2, first2 + (last1 - first1)) with respect to a specified
 * predicate, and false otherwise.
 *
 * \ingroup     DashAlgorithms
 */
template <typename GlobIter, class BinaryPredicate>
bool equal(
    /// Iterator to the initial position in the sequence
    GlobIter        first_1,
    /// Iterator to the final position in the sequence
    GlobIter        last_1,
    GlobIter        first_2,
    BinaryPredicate pred)
{
  static_assert(
      dash::iterator_traits<GlobIter>::is_global_iterator::value,
      "invalid iterator: Need to be a global iterator");

  auto & team        = first_1.team();
  auto myid          = team.myid();
  // Global iterators to local range:
  auto index_range   = dash::local_range(first_1, last_1);
  auto l_first_1     = index_range.begin;
  auto l_last_1      = index_range.end;
  char l_result      = static_cast<char>(std::equal(l_first_1, l_last_1, first_2, pred));
  char r_result      = 0;

  DASH_ASSERT_RETURNS(
    dart_allreduce(&l_result, &r_result, 1,
      DART_TYPE_BYTE, DART_OP_BAND, team.dart_id()),
    DART_OK);
  return r_result;
}

} // namespace dash

#endif // DASH__ALGORITHM__EQUAL_H__
