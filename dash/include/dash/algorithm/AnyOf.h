#ifndef DASH__ALGORITHM__ANY_OF_H__
#define DASH__ALGORITHM__ANY_OF_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/Find.h>


namespace dash {

/**
 * Check whether any element in the range satisfies predicate \c p.
 *
 * \returns \c true if at least one element satisfies \c p, \c false otherwise.
 *
 * \see dash::find_if
 * \see dash::find_if_not
 * \see dash::all_of
 * \ingroup DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
  typename UnaryPredicate >
bool any_of(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Predicate applied to the elements in range [first, last)
  UnaryPredicate                       p)
{
  return find_if(first, last, p) != last;
}

} // namespace dash

#endif // DASH__ALGORITHM__ANY_OF_H__
