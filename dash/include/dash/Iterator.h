#ifndef DASH__ITERATOR_H__INCLUDED
#define DASH__ITERATOR_H__INCLUDED

#include <dash/Types.h>
#include <dash/iterator/GlobIter.h>
#include <dash/iterator/GlobViewIter.h>

#include <iterator>


namespace dash {

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
 */
template<
  typename ElementType,
  class    Pattern,
  class    GlobMem,
  class    Pointer,
  class    Reference >
auto distance(
  const GlobIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    first,
  /// Global iterator to the final position in the global sequence
  const GlobIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    last)
-> typename Pattern::index_type
{
  return last - first;
}

template <class T>
constexpr std::ptrdiff_t distance(T * first, T * last) {
  return std::distance(first, last);
}

/**
 * Resolve the number of elements between two global pointers.
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *
 * \code
 *   g_last == g_first + (l_last - l_first)
 * \endcode
 *
 * \code
 * Example:
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * \endcode
 *
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<typename ElementType>
dash::default_index_t distance(
  /// Global pointer to the initial position in the global sequence
  dart_gptr_t first,
  /// Global pointer to the final position in the global sequence
  dart_gptr_t last)
{
  GlobPtr<ElementType> & gptr_first(first);
  GlobPtr<ElementType> & gptr_last(last);
  return gptr_last - gptr_first;
}

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
