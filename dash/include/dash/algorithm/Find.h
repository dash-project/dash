#ifndef DASH__ALGORITHM__FIND_H__
#define DASH__ALGORITHM__FIND_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/**
 * Returns an iterator to the first element in the range \c [first,last) that
 * compares equal to \c val.
 * If no such element is found, the function returns \c last.
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> find(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType> first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType> last,
  /// Value which will be assigned to the elements in range [first, last)
  const ElementType                  & value)
{
  using p_index_t = typename PatternType::index_type;
  
  if(first >= last){
    return last;
  }

  p_index_t g_index;
  auto index_range   = dash::local_index_range(first, last);
  auto l_begin_index = index_range.begin;
  auto l_end_index   = index_range.end;
  auto & pattern     = first.pattern();
  auto & team        = pattern.team();
  auto first_offset  = first.pos();
  if(l_begin_index == l_end_index){
    g_index = std::numeric_limits<p_index_t>::max();
  } else {
   auto g_begin_index = pattern.global(l_begin_index);
  
    // Pointer to first element in local memory:
    const ElementType * lbegin        = first.globmem().lbegin(
                                           team.myid());
    // Pointers to first / final element in local range:
    const ElementType * l_range_begin = lbegin + l_begin_index;
    const ElementType * l_range_end   = lbegin + l_end_index;
  
    DASH_LOG_DEBUG("local index range", l_begin_index, l_end_index);
  
    auto l_result = std::find(l_range_begin, l_range_end, value);
    auto l_hit_index  = l_result - lbegin;
    if(l_result == l_range_end){
      DASH_LOG_DEBUG("Not found in local range");
      g_index = std::numeric_limits<p_index_t>::max();
    } else {
      g_index = pattern.global(l_hit_index); 
    }
  }
  // dist array containing local results
  std::vector<p_index_t> local_min_indices(team.size());
  DASH_ASSERT_RETURNS(
      dart_allgather(&g_index,
                      local_min_indices.data(),
                      sizeof(p_index_t),
                      team.dart_id()),
      DART_OK);

  auto g_min_it = std::min_element(local_min_indices.begin(), local_min_indices.end());
  if((*g_min_it) == std::numeric_limits<p_index_t>::max()){
    DASH_LOG_DEBUG("element not found");
    return last;
  }

  return first + (*g_min_it);

#if 0
  typedef dash::default_index_t index_t;

  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first, last);
  auto l_first       = index_range.begin;
  auto l_last        = index_range.end;
  auto first_offset  = first.pos();
  auto pattern       = first.pattern();

  auto l_result      = std::find(l_first, l_last, value);
  auto l_offset      = std::distance(l_first, l_result);
  if (l_result == l_last) {
    l_offset = -1;
  }
  auto g_offset      = pattern.global(l_offset);
  dash::Array<decltype(g_offset)> l_results(first.pattern().team().size(), first.pattern().team());

  l_results.local[0] = g_offset;
  l_results.barrier();

  auto g_result_idx = dash::min_element(l_results.begin(), l_results.end());

  return l_result; //(first + (g_result_idx - first_offset)).get();
#endif
}

/**
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
	class    UnaryPredicate >
GlobIter<ElementType, PatternType> find_if(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Predicate which will be applied to the elements in range [first, last)
	UnaryPredicate                       predicate)
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

  dash::Array<dart_unit_t> l_results(dash::size());

  l_results.local[0] = l_offset;

  dash::barrier();

  // All local offsets stored in l_results

  for (auto u = 0; u < dash::size(); u++) {
    if (static_cast<index_t>(l_results[u]) >= 0) {
      auto g_offset = first.pattern()
                           .global_index(
                              u,
                              { static_cast<index_t>(l_results[u]) });
      return first + g_offset - first.pos();
    }
  }

  return last;
}

/**
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
	class    UnaryPredicate>
GlobIter<ElementType, PatternType> find_if_not(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Predicate which will be applied to the elements in range [first, last)
	UnaryPredicate                       predicate)
{
  return find_if(first, last, std::not1(predicate));
}

} // namespace dash

#endif // DASH__ALGORITHM__FIND_H__
