#ifndef DASH__ALGORITHM__EQUAL_H__
#define DASH__ALGORITHM__EQUAL_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {
namespace internal {
  template<typename Ptr>
  inline char equal_loc_impl(
      const Ptr & l_first_1,
      const Ptr & l_last_1,
      const Ptr & l_first_2){
    return static_cast<char>(std::equal(l_first_1, l_last_1, l_first_2));
  }
  template<typename Ptr, typename BinaryPredicate>
  inline char equal_loc_impl(
      const Ptr & l_first_1,
      const Ptr & l_last_1,
      const Ptr & l_first_2,
      BinaryPredicate pred){
    return static_cast<char>(std::equal(l_first_1, l_last_1, l_first_2, pred));
  }

  template<typename GlobIter>
  char equal_overlapping_impl(
      const GlobIter & first_1,
      const GlobIter & last_1,
      const GlobIter & first_2){
    auto loc_idx_r   = dash::local_index_range(first_1, last_1);
    auto dist        = loc_idx_r.end - loc_idx_r.begin;
    auto git_beg_idx = first_1.pattern().global(loc_idx_r.begin);
    auto offset      = git_beg_idx - first_1.gpos();
    auto glfirst_1   = first_1+offset;  // globIt to first pos

    // TODO: This should work for one-local-block ranges, but does not
    // KNOWN ISSUE
    return std::equal(glfirst_1, glfirst_1+dist, first_2+offset);
  }
}

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
  auto index_range_in   = dash::local_range(first_1, last_1);
  auto l_first_1        = index_range_in.begin;
  auto l_last_1         = index_range_in.end;
  auto dist             = std::distance(l_first_1, l_last_1);
  auto index_range_out  = dash::local_range(first_2, first_2 + dist);
  auto common_dist      = std::min(std::distance(index_range_out.begin, index_range_out.end),dist);
  char l_result         = 1;

  // check if local ranges are corresponding
  if(common_dist == dist){
    l_result = ::dash::internal::equal_loc_impl(l_first_1, l_last_1, first_2.local());
  } else if(common_dist > 0) {
    l_result = ::dash::internal::equal_overlapping_impl(
        first_1, last_1, first_2);
  }

  char r_result         = 0;

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
  auto index_range_in   = dash::local_range(first_1, last_1);
  auto l_first_1        = index_range_in.begin;
  auto l_last_1         = index_range_in.end;
  auto dist             = std::distance(l_first_1, l_last_1);
  auto index_range_out  = dash::local_range(first_2, first_2 + dist);
  auto common_dist      = std::min(std::distance(index_range_out.begin, index_range_out.end),dist);
  char l_result         = 1;

  // check if local ranges are corresponding
  if(std::distance(index_range_out.begin, index_range_out.end) == dist){
    l_result = ::dash::internal::equal_loc_impl(l_first_1, l_last_1, first_2.local());
  } else if(common_dist > 0) {
    l_result = ::dash::internal::equal_overlapping_impl(
        first_1, last_1, first_2);
  }

  char r_result      = 0;

  DASH_ASSERT_RETURNS(
    dart_allreduce(&l_result, &r_result, 1,
      DART_TYPE_BYTE, DART_OP_BAND, team.dart_id()),
    DART_OK);
  return r_result;
}

} // namespace dash

#endif // DASH__ALGORITHM__EQUAL_H__
