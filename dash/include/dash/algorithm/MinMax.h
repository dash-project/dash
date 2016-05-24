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
  typedef typename pattern_t::index_type              index_t;

  // return last for empty array
  if (first == last) {
    DASH_LOG_DEBUG("dash::min_element >",
                   "empty range, returning last", last);
    return last;
  }

  auto & pattern = first.pattern();

  dash::Team & team = pattern.team();
  DASH_LOG_DEBUG("dash::min_element()",
                 "allocate minarr, size", team.size());
  // Global position of end element in range:
  auto    gi_last            = last.gpos();
  // Find the local min. element in parallel
  // Get local address range between global iterators:
  auto    local_idx_range    = dash::local_index_range(first, last);
  // Pointer to local minimum element:
  const   ElementType * lmin = nullptr;
  // Local offset of local minimum element, or -1 if no element found:
  index_t l_idx_lmin         = -1;
  if (local_idx_range.begin == local_idx_range.end) {
    // local range is empty
    DASH_LOG_DEBUG("dash::min_element", "local range empty");
  } else {
    // Pointer to first element in local memory:
    const ElementType * lbegin        = first.globmem().lbegin(
                                          pattern.team().myid());
    // Pointers to first / final element in local range:
    const ElementType * l_range_begin = lbegin + local_idx_range.begin;
    const ElementType * l_range_end   = lbegin + local_idx_range.end;
    lmin = ::std::min_element(l_range_begin,
                              l_range_end,
                              compare);
    if (lmin != l_range_end) {
      DASH_LOG_TRACE_VAR("dash::min_element", *lmin);
      // Offset of local minimum in local memory:
      l_idx_lmin = lmin - lbegin;
    }
  }
  DASH_LOG_TRACE("dash::min_element",
                 "local index of local minimum:", l_idx_lmin);
  DASH_LOG_TRACE("dash::min_element",
                 "waiting for local min of other units");
  team.barrier();

  typedef struct {
    ElementType value;
    index_t     g_index;
  } local_min_t;

  std::vector<local_min_t> local_min_values(team.size());

  // Set global index of local minimum to -1 if no local minimum has been
  // found:
  local_min_t local_min;
  local_min.value   = l_idx_lmin < 0
                      ? ElementType()
                      : *lmin;
  local_min.g_index = l_idx_lmin < 0
                      ? -1
                      : pattern.global(l_idx_lmin);

  DASH_LOG_TRACE("dash::min_element", "sending local minimum: {",
                 "value:", local_min.value, "index:", local_min.l_index, "}");

  DASH_LOG_DEBUG("dash::min_element: dart_allgather");
  DASH_ASSERT_RETURNS(
    dart_allgather(
      &local_min, local_min_values.data(), sizeof(local_min_t),
      team.dart_id()),
    DART_OK);

  auto gmin_elem_it  = ::std::min_element(
                           local_min_values.begin(),
                           local_min_values.end(),
                           [&](local_min_t & a, local_min_t & b) {
                             // Ignore elements with global index -1 (no
                             // element found):
                             return (a.g_index > 0 &&
                                     compare(a.value, b.value));
                           });

  if (gmin_elem_it == local_min_values.end()) {
    DASH_LOG_DEBUG_VAR("dash::min_element >", last);
    return last;
  }

  auto min_elem_unit = static_cast<dart_unit_t>(
                         gmin_elem_it - local_min_values.begin());
  auto gi_minimum    = gmin_elem_it->g_index;

  DASH_LOG_TRACE("dash::min_element",
                 "min. value:", gmin_elem_it->value,
                 "at unit:",    min_elem_unit,
                 "global idx:", gi_minimum);

  DASH_LOG_TRACE_VAR("dash::min_element", gi_minimum);
  if (gi_minimum == gi_last) {
    DASH_LOG_DEBUG_VAR("dash::min_element >", last);
    return last;
  }
  // iterator 'first' is relative to start of input range, convert to start
  // of its referenced container (= container.begin()), then apply global
  // offset of minimum element:
  globiter_t minimum = (first - first.gpos()) + gi_minimum;
  DASH_LOG_DEBUG("dash::min_element >", minimum,
                 "=", static_cast<ElementType>(*minimum));
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
