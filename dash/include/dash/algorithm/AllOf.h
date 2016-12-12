#ifndef DASH__ALGORITHM__ALL_OF_H__
#define DASH__ALGORITHM__ALL_OF_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/Find.h>


namespace dash {

/**
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
	typename UnaryPredicate>
GlobIter<ElementType, PatternType> all_of(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Predicate applied to the elements in range [first, last)
  UnaryPredicate p)
{
  return find_if_not(first, last, p) == last;
}

} // namespace dash

#endif // DASH__ALGORITHM__ALL_OF_H__
