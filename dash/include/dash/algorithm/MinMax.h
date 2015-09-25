#ifndef DASH__ALGORITHM__MIN_MAX_H__
#define DASH__ALGORITHM__MIN_MAX_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/internal/Logging.h>

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
 * \ingroup     Algorithms
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
  DASH_LOG_DEBUG("min_element",
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
    DASH_LOG_TRACE_VAR("min_element", lend - lbegin);
    DASH_LOG_TRACE_VAR("min_element", lmin - lbegin);
    DASH_LOG_DEBUG_VAR("min_element", *lmin);
    minarr[team.myid()] = first.globmem().index_to_gptr(
                            team.myid(),
                            lmin - lbegin);
  }
  team.barrier();
  dash::Shared<globptr_t> min; 
  // find the global min. element
  if (team.myid() == 0) {
    globptr_t minloc   = minarr[0];
    ElementType minval = *minloc;
    for (auto i = 1; i < minarr.size(); ++i) {
      globptr_t lmingptr = static_cast<globptr_t>(minarr[i]);
      // Local gptr of units might be null if unit
      // had empty range
      if (lmingptr != nullptr) {
        ElementType val = *lmingptr;
        if (compare(val, minval)) {
          minloc = lmingptr;
          DASH_LOG_TRACE("Array.min_element", 
                         "Setting min val to", val);
          minval = val;
        }
      }
    }
    min.set(minloc);
  }
  team.barrier();
  // Minimum has been set by unit 0 at this point
  globptr_t minimum = min.get();
  if (minimum == nullptr) {
    return last;
  }
  return minimum;
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
 * \ingroup     Algorithms
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

} // namespace dash

#endif // DASH__ALGORITHM__MIN_MAX_H__
