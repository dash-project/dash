#ifndef DASH__ALGORITHM__MIN_MAX_H__
#define DASH__ALGORITHM__MIN_MAX_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/internal/Logging.h>
#include <dash/Array.h>

#include <algorithm>

namespace dash {

/**
 * Finds an iterator pointing to the element with the smallest value in
 * the range [first,last).
 *
 * \return      An iterator to the first occurrence of the smallest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> min_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::less<const ElementType &>())
{
  typedef dash::GlobIter<ElementType, PatternType> globiter_t;
  typedef dash::GlobPtr<ElementType, PatternType>   globptr_t;
  typedef PatternType                               pattern_t;
  typedef typename pattern_t::size_type              extent_t;

  auto & pattern = first.pattern();

  dash::Team & team = pattern.team();
  DASH_LOG_DEBUG("dash::min_element()",
                 "allocate minarr, size", team.size());
  dash::Array<globiter_t> minarr(team.size());
  // return last for empty array
  if (first == last) {
    return last;
  }
  // Find the local min. element in parallel
  // Get local address range between global iterators:
  auto local_idx_range = dash::local_index_range(first, last);
  if (local_idx_range.begin == local_idx_range.end) {
    // local range is empty
    minarr[team.myid()] = last;
    DASH_LOG_DEBUG("dash::min_element", "local range empty");
  } else {
    // Pointer to first element in local memory:
    const ElementType * lbegin        = first.globmem().lbegin(
                                          pattern.team().myid());
    // Pointers to first / final element in local range:
    const ElementType * l_range_begin = lbegin + local_idx_range.begin;
    const ElementType * l_range_end   = lbegin + local_idx_range.end;
    const ElementType * lmin          = ::std::min_element(l_range_begin,
                                                           l_range_end,
                                                           compare);
    // Offset of local minimum in local memory:
    auto l_idx_lmin = lmin - lbegin;
    DASH_LOG_TRACE_VAR("dash::min_element", l_idx_lmin);
    if (lmin != l_range_end) {
      DASH_LOG_TRACE_VAR("dash::min_element", *lmin);
    }
#if 0
    // Global pointer to local minimum:
    globptr_t g_lmin(first.globmem().index_to_gptr(
                                       team.myid(),
                                       l_idx_lmin));
#else
    // Global iterator to local minimum.
    // Convert 'first' to container.begin(), then add global canonical index
    // of local minimum:
    auto g_lmin = first - first.gpos() + pattern.global(l_idx_lmin);
#endif
    if (lmin != l_range_end) {
      DASH_LOG_DEBUG("dash::min_element", g_lmin,
                     "=", static_cast<ElementType>(*g_lmin));
    }
    minarr[team.myid()] = g_lmin;
  }
  DASH_LOG_TRACE("dash::min_element", "waiting for local min of other units");
  team.barrier();
  // Shared global pointer referencing element with global minimum:
  dash::Shared<globiter_t> shared_min;
  // Find the global min. element:
  if (team.myid() == 0) {
    DASH_LOG_TRACE("dash::min_element", "finding global min");
    auto        minloc = last;
    ElementType minval = ElementType();
    for (extent_t i = 0; i < minarr.size(); ++i) {
      globiter_t g_lmin = minarr[i];
      DASH_LOG_TRACE("dash::min_element", "unit:", i, "g_lmin:", g_lmin);
      // Local gptr of units might be null if unit had empty range:
      if (g_lmin != last) {
        ElementType val = *g_lmin;
        DASH_LOG_TRACE("dash::min_element",
                       "local min of unit", i, ": ", val);
        if (minloc == last || compare(val, minval)) {
          DASH_LOG_TRACE("dash::min_element",
                         "setting current minval to", val);
          minloc = g_lmin;
          minval = val;
        }
      }
    }
    DASH_LOG_TRACE("dash::min_element",
                   "setting global min gptr to", minloc);
    shared_min.set(minloc);
  }
  // Wait for unit 0 to resolve global minimum
  team.barrier();
  // Minimum has been set by unit 0 at this point
  globiter_t minimum = shared_min.get();
  if (minimum == last) {
    DASH_LOG_DEBUG("dash::min_element >", last);
    return last;
  }
  DASH_LOG_DEBUG("dash::min_element >", minimum,
                 "=", (ElementType)(*minimum));
  return minimum;
}

/**
 * Finds an iterator pointing to the element with the smallest value in
 * the range [first,last).
 * Specialization for local range, delegates to std::min_element.
 *
 * \return      An iterator to the first occurrence of the smallest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<typename ElementType>
const ElementType * min_element(
  /// Iterator to the initial position in the sequence
  const ElementType * first,
  /// Iterator to the final position in the sequence
  const ElementType * last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::less<const ElementType &>())
{
  // Same as min_element with different compare function
  return std::min_element(first, last, compare);
}

/**
 * Finds an iterator pointing to the element with the greatest value in
 * the range [first,last).
 *
 * \return      An iterator to the first occurrence of the greatest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> max_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::greater<const ElementType &>())
{
  // Same as min_element with different compare function
  return dash::min_element(first, last, compare);
}

/**
 * Finds an iterator pointing to the element with the greatest value in
 * the range [first,last).
 * Specialization for local range, delegates to std::min_element.
 *
 * \return      An iterator to the first occurrence of the greatest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template< typename ElementType >
const ElementType * max_element(
  /// Iterator to the initial position in the sequence
  const ElementType * first,
  /// Iterator to the final position in the sequence
  const ElementType * last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::greater<const ElementType &>())
{
  // Same as min_element with different compare function
  return std::min_element(first, last, compare);
}

} // namespace dash

#endif // DASH__ALGORITHM__MIN_MAX_H__
