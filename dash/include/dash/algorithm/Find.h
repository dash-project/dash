#ifndef DASH__ALGORITHM__FIND_H__
#define DASH__ALGORITHM__FIND_H__

#include <dash/Array.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/iterator/GlobIter.h>

namespace dash {

/**
 * Returns an iterator to the first element in the range \c [first,last) that
 * compares equal to \c val.
 * If no such element is found, the function returns \c last.
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename GlobIter,
  typename ElementType>
GlobIter find(
  /// Iterator to the initial position in the sequence
  GlobIter   first,
  /// Iterator to the final position in the sequence
  GlobIter   last,
  /// Value which is searched for using operator==
  const ElementType & value)
{

  //use iterator traits
  using iterator_traits = dash::iterator_traits<GlobIter>;

  using p_index_t = typename iterator_traits::index_type;

  if(first >= last) {
    return last;
  }

  p_index_t g_index;
  auto & pattern     = first.pattern();
  auto & team        = pattern.team();
  auto index_range   = dash::local_index_range(first, last);
  auto l_begin_index = index_range.begin;
  auto l_end_index   = index_range.end;
  auto first_offset  = first.pos();
  if(l_begin_index == l_end_index){
    g_index = std::numeric_limits<p_index_t>::max();
  } else {
   auto g_begin_index = pattern.global(l_begin_index);

    // Pointer to first element in local memory:
    const ElementType * lbegin        = first.globmem().lbegin();
    // Pointers to first / final element in local range:
    const ElementType * l_range_begin = lbegin + l_begin_index;
    const ElementType * l_range_end   = lbegin + l_end_index;

    DASH_LOG_DEBUG("local index range", l_begin_index, l_end_index);

    auto l_result = std::find(l_range_begin, l_range_end, value);
    if(l_result == l_range_end){
      DASH_LOG_DEBUG("Not found in local range");
      g_index = std::numeric_limits<p_index_t>::max();
    } else {
      auto l_hit_index = l_result - lbegin;
      g_index = pattern.global(l_hit_index);
    }
  }
  team.barrier();

  // receive buffer for global maximal index
  p_index_t g_hit_idx;

  DASH_ASSERT_RETURNS(
      dart_allreduce(
        &g_index,
        &g_hit_idx,
        1,
        dart_datatype<p_index_t>::value,
        DART_OP_MIN,
        team.dart_id()),
      DART_OK);

  if (g_hit_idx == std::numeric_limits<p_index_t>::max()) {
    DASH_LOG_DEBUG("element not found");
  } else {
    return first + g_hit_idx;
  }
  return last;
}

/**
 * Returns an iterator to the first element in the range \c [first,last) that
 * satisfies the predicate \c p.
 * If no such element is found, the function returns \c last.
 *
 * \see dash::find
 * \see dash::find_if_not
 *
 * \ingroup     DashAlgorithms
 */
template <typename GlobIter, typename UnaryPredicate>
GlobIter find_if(
    /// Iterator to the initial position in the sequence
    GlobIter first,
    /// Iterator to the final position in the sequence
    GlobIter last,
    /// Predicate which will be applied to the elements in range [first, last)
    UnaryPredicate predicate)
{
  using iterator_traits = dash::iterator_traits<GlobIter>;

  using index_t = typename iterator_traits::index_type;

  auto& team = first.pattern().team();
  auto  myid = team.myid();
  /// Global iterators to local range:
  auto index_range = dash::local_range(first, last);
  auto l_first     = index_range.begin;
  auto l_last      = index_range.end;

  auto l_result = std::find_if(l_first, l_last, predicate);
  auto l_offset = l_result == l_last ? -1 : std::distance(l_first, l_result);

  dash::Array<index_t> l_results(team.size(), team);

  l_results.local[0] = l_offset;

  team.barrier();

  // All local offsets stored in l_results
  auto result = last;

  for (auto u = 0; u < team.size(); u++) {
    if (l_results[u] >= 0) {
      auto g_offset = first.pattern().global_index(u, {l_results[u]});
      result        = first + g_offset - first.pos();
      break;
    }
  }

  team.barrier();
  return result;
}

/**
 * Returns an iterator to the first element in the range \c [first,last) that
 * does not satisfy the predicate \c p.
 * If no such element is found, the function returns \c last.
 *
 * \see dash::find
 * \see dash::find_if_not
 *
 * \ingroup     DashAlgorithms
 */
template <
    typename GlobIter,
    class UnaryPredicate>
GlobIter find_if_not(
    /// Iterator to the initial position in the sequence
    GlobIter first,
    /// Iterator to the final position in the sequence
    GlobIter last,
    /// Predicate which will be applied to the elements in range [first, last)
    UnaryPredicate predicate)
{
  return find_if(first, last, std::not1(predicate));
}

} // namespace dash

#endif // DASH__ALGORITHM__FIND_H__
