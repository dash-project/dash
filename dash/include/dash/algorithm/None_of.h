#ifndef DASH__ALGORITHM__NONE_OF_H__
#define DASH__ALGORITHM__NONE_OF_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/**
 * \ingroup     DashAlgorithms
 */
template<
    typename ElementType,
    class PatternType>
GlobIter<ElementType, PatternType> any_of(
    /// Iterator to the initial position in the sequence
    GlobIter<ElementType, PatternType>   first,
    /// Iterator to the final position in the sequence
    GlobIter<ElementType, PatternType>   last,
    /// Predicate applied to the elements in range [first, last)
    UnaryPredicate p)
{
    
    return find_if(first, last, p) == last;

} // namespace dash

#endif // DASH__ALGORITHM__NONE_OF_H__
