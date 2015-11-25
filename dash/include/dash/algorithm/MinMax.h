#ifndef DASH__ALGORITHM__MIN_MAX_H__
#define DASH__ALGORITHM__MIN_MAX_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/internal/Logging.h>

#include<algorithm>

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
  class PatternType>
GlobPtr<ElementType> min_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
      bool(const ElementType &, const ElementType)
    > & compare
      = std::less<const ElementType &>()) {
  typedef dash::GlobPtr<ElementType> globptr_t;
  auto pattern      = first.pattern();
  dash::Team & team = pattern.team();
  DASH_LOG_DEBUG("min_element()",
                 "allocate minarr, size", team.size());
  dash::Array<globptr_t> minarr(team.size());
  // return last for empty array
  if (first == last) {
    return last;
  }
  // Find the local min. element in parallel
  // Get local address range between global iterators:
  auto local_range           = dash::local_range(first, last);
  const ElementType * lbegin = local_range.begin;
  const ElementType * lend   = local_range.end;
  if (lbegin == lend || lbegin == nullptr || lend == nullptr) {
    // local range is empty
    minarr[team.myid()] = nullptr;
    DASH_LOG_DEBUG("min_element", "local range empty");
  } else {
    const ElementType * lmin = 
      ::std::min_element(lbegin, lend, compare);
    // Local index of local minimum:
    auto lidx_lmin = lmin - lbegin;
    DASH_LOG_TRACE_VAR("min_element", lend - lbegin);
    DASH_LOG_TRACE_VAR("min_element", lidx_lmin);
    DASH_LOG_DEBUG_VAR("min_element", *lmin);
    // Global pointer to local minimum:
    auto gptr_lmin = first.globmem().index_to_gptr(
                                       team.myid(),
                                       lidx_lmin);
    DASH_LOG_DEBUG_VAR("min_element", gptr_lmin);
    minarr[team.myid()] = gptr_lmin;
  }
  DASH_LOG_TRACE("min_element", "Waiting for local min of other units");
  team.barrier();
  // Shared global pointer referencing element with global minimum:
  dash::Shared<globptr_t> min; 
  // Find the global min. element:
  if (team.myid() == 0) {
    DASH_LOG_TRACE("min_element", "Finding global min");
    globptr_t minloc   = static_cast<globptr_t>(minarr[0]);
    DASH_LOG_TRACE_VAR("min_element", minloc);
    ElementType minval = *minloc;
    for (auto i = 1; i < minarr.size(); ++i) {
      DASH_LOG_TRACE("min_element", "Comparing local min of unit", i);
      globptr_t lmingptr = static_cast<globptr_t>(minarr[i]);
      DASH_LOG_TRACE_VAR("min_element", lmingptr);
      // Local gptr of units might be null if unit
      // had empty range
      if (lmingptr != nullptr) {
        ElementType val = *lmingptr;
        if (compare(val, minval)) {
          minloc = lmingptr;
          DASH_LOG_TRACE("min_element", "Setting current minval to", val);
          minval = val;
        }
      }
    }
    DASH_LOG_TRACE("min_element", "Setting global min gptr to", minloc);
    min.set(minloc);
  }
  // Wait for unit 0 to resolve global minimum
  team.barrier();
  // Minimum has been set by unit 0 at this point
  globptr_t minimum = min.get();
  if (minimum == nullptr) {
    return last;
  }
  DASH_LOG_DEBUG("min_element >", minimum);
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
template< typename ElementType >
const ElementType * min_element(
  /// Iterator to the initial position in the sequence
  const ElementType * first,
  /// Iterator to the final position in the sequence
  const ElementType * last,
  /// Element comparison function, defaults to std::less
  const std::function<
      bool(const ElementType &, const ElementType)
    > & compare
      = std::less<const ElementType &>()) {
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
  class PatternType>
GlobPtr<ElementType> max_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
      bool(const ElementType &, const ElementType)
    > & compare
      = std::greater<const ElementType &>()) {
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
      = std::greater<const ElementType &>()) {
  // Same as min_element with different compare function
  return std::min_element(first, last, compare);
}

} // namespace dash

#endif // DASH__ALGORITHM__MIN_MAX_H__
