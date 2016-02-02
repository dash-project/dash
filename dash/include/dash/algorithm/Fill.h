#ifndef DASH__ALGORITHM__FILL_H__
#define DASH__ALGORITHM__FILL_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

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
  class PatternType>
void fill(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Value which will be assigned to the elements in range [first, last)
  const ElementType & value) {
  /// Global iterators to local index range:
  auto index_range  = dash::local_index_range(first, last);
  auto lbegin_index = index_range.begin;
  auto lend_index   = index_range.end;
  if (lbegin_index == lend_index) {
    // Local range is empty
    return;
  }
  // Iterate local index range:
  for (ElementType el = first;
       el != last;
       ++el) {
    *el = value;
  }
}

} // namespace dash

#endif // DASH__ALGORITHM__TRANSFORM_H__
