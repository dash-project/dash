#ifndef DASH__ALGORITHM__FOR_EACH_H__
#define DASH__ALGORITHM__FOR_EACH_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/internal/Logging.h>

namespace dash {

/**
 * Invoke a function on every element in a range distributed by a pattern.
 * Being a collaborative operation, each unit will invoke the given
 * function on its local elements only.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \tparam      IndexType    Parameter type expected by function to
 *                           invoke, deduced from parameter \c func
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  typename IndexType,
  class PatternType>
void for_each(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Function to invoke on every index in the range
  ::std::function<void(IndexType)> & func) {
  /// Global iterators to local index range:
  auto index_range  = dash::local_index_range(first, last);
  auto lbegin_index = index_range.begin;
  auto lend_index   = index_range.end;
  if (lbegin_index == lend_index) {
    // Local range is empty
    return;
  }
  // Pattern from global begin iterator:
  auto pattern = first.pattern();
  // Iterate local index range:
  for (IndexType lindex = lbegin_index;
       lindex != lend_index;
       ++lindex) {
    IndexType gindex = pattern.local_to_global_index(lindex);
    func(gindex);
  }
}

/**
 * Invoke a function on every element in a range distributed by a pattern.
 * Being a collaborative operation, each unit will invoke the given
 * function on its local elements only.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \tparam      IndexType    Parameter type expected by function to
 *                           invoke, deduced from parameter \c func
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  typename IndexType,
  class PatternType>
void for_each_with_index(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Function to invoke on every index in the range
  ::std::function<void(const ElementType &, IndexType)> & func) {
  /// Global iterators to local index range:
  auto index_range  = dash::local_index_range(first, last);
  auto lbegin_index = index_range.begin;
  auto lend_index   = index_range.end;
  if (lbegin_index == lend_index) {
    // Local range is empty
    return;
  }
  // Pattern from global begin iterator:
  auto pattern = first.pattern();
  // Iterate local index range:
  for (IndexType lindex = lbegin_index;
       lindex != lend_index;
       ++lindex) {
    IndexType gindex  = pattern.local_to_global_index(lindex);
    auto first_offset = first.pos();
    auto element_it   = first + (gindex - first_offset);
    func(*element_it, gindex);
  }
}

} // namespace dash

#endif // DASH__ALGORITHM__FOR_EACH_H__
