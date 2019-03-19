#ifndef DASH__ALGORITHM__ALL_OF_H__
#define DASH__ALGORITHM__ALL_OF_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/Find.h>


namespace dash {


/**
 * Check whether all element in the range satisfy predicate \c p.
 *
 * \returns \c true if all elements satisfy \c p, \c false otherwise.
 *
 * \see dash::find_if
 * \see dash::find_if_not
 * \see dash::any_of
 * \ingroup DashAlgorithms
 */
template<
  typename GlobIter,
  typename UnaryPredicate>
bool all_of(
  /// Iterator to the initial position in the sequence
  GlobIter   first,
  /// Iterator to the final position in the sequence
  GlobIter   last,
  /// Predicate applied to the elements in range [first, last)
  UnaryPredicate p)
{
  return find_if_not(first, last, p) == last;
}

} // namespace dash

#endif // DASH__ALGORITHM__ALL_OF_H__
