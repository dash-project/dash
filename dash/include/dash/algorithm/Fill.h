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
  GlobIter<ElementType, PatternType>  first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>  last,
  /// Value which will be assigned to the elements in range [first, last)
  const ElementType & value) {
  /// Global iterators to local range:
  auto index_range  = dash::local_range(first, last);
  ElementType * lfirst = index_range.begin;
  ElementType * llast = index_range.end;

  std::fill(lfirst, llast, value);
}

} // namespace dash

#endif // DASH__ALGORITHM__TRANSFORM_H__
