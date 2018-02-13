#ifndef DASH__ITERATOR_H__INCLUDED
#define DASH__ITERATOR_H__INCLUDED

#include <dash/Types.h>
#include <dash/Dimensional.h>
#include <dash/iterator/IteratorTraits.h>
#include <dash/iterator/GlobIter.h>
#include <dash/iterator/GlobViewIter.h>

#include <iterator>

/**
 * \defgroup  DashIteratorConcept  Multidimensional Iterator Concept
 *
 * \ingroup DashNDimConcepts
 * \{
 * \par Description
 *
 * Definitions for multidimensional iterator expressions.
 *
 * \see DashDimensionalConcept
 * \see DashViewConcept
 * \see DashRangeConcept
 *
 * \see \c dash::view_traits
 *
 * \par Types
 *
 * \par Expressions
 *
 * Expression               | Returns | Effect | Precondition | Postcondition
 * ------------------------ | ------- | ------ | ------------ | -------------
 *
 * \par Metafunctions
 *
 * - \c dash::iterator_traits<I>
 *
 * \par Functions
 *
 * - \c dash::index
 *
 * \par Functions in the Range concept
 *
 * - \c dash::distance
 * - \c dash::size
 *
 * \}
 */

namespace dash {


/**
 *
 * \concept{DashIteratorConcept}
 */
template <class IndexType>
constexpr typename std::enable_if<
  std::is_integral<IndexType>::value, IndexType >::type
index(IndexType idx) {
  return idx;
}

/**
 *
 * \concept{DashIteratorConcept}
 */
// template <class Iterator>
// constexpr auto index(Iterator it) -> decltype((++it).pos()) {
//   return it.pos();
// }

/**
 *
 * \concept{DashIteratorConcept}
 */
template <class Iterator>
constexpr auto index(Iterator it) -> decltype((++it).gpos()) {
  return it.gpos();
}


/**
 * Resolve the number of elements between two iterators.
 *
 * \concept{DashIteratorConcept}
 */
template <class RandomAccessIt>
typename RandomAccessIt::difference_type
distance(
  const RandomAccessIt & first,
  const RandomAccessIt & last)
{
  return last - first;
}

/**
 * Resolve the number of elements between two global iterators.
 *
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *   g_last == g_first + (l_last - l_first)
 * Example:
 *
 * \code
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * \endcode
 *
 * When iterating in local memory range [0,5[ of unit 0, the position of
 * the global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 *
 * \concept{DashIteratorConcept}
 */
template<
  typename ElementType,
  class    Pattern,
  class    GlobMemType,
  class    Pointer,
  class    Reference >
typename Pattern::index_type
distance(
  /// Global pointer to the initial position in the global sequence
  const GlobIter<ElementType, Pattern, GlobMemType, Pointer, Reference> &
    first,
  /// Global iterator to the final position in the global sequence
  const GlobIter<ElementType, Pattern, GlobMemType, Pointer, Reference> &
    last)
{
  return last - first;
}

/**
 *
 * \ingroup     Algorithms
 *
 * \concept{DashIteratorConcept}
 */
template <class T>
constexpr std::ptrdiff_t distance(T * const first, T * const last) {
  return std::distance(first, last);
}

/**
 *
 * \ingroup     Algorithms
 *
 * \concept{DashIteratorConcept}
 */
template <
  class OffsetType >
constexpr typename std::enable_if<
  std::is_integral<OffsetType>::value,
  OffsetType >::type
distance(
  OffsetType begin,
  OffsetType end) {
  return (end - begin);
}

} // namespace dash

#endif // DASH__ITERATOR_H__INCLUDED
