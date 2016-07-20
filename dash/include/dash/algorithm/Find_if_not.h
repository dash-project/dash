#ifndef DASH__ALGORITHM__FIND_IF_NOT_H__
#define DASH__ALGORITHM__FIND_IF_NOT_H__

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
    class PatternType,
	class UnaryPredicate>
GlobIter<ElementType, PatternType> find_if_not(
    /// Iterator to the initial position in the sequence
    GlobIter<ElementType, PatternType>   first,
    /// Iterator to the final position in the sequence
    GlobIter<ElementType, PatternType>   last,
    /// Predicate which will be applied to the elements in range [first, last)
	UnaryPredicate predicate)
{
    return find_if(first, last, std::not1(predicate));
  
}

} // namespace dash

#endif // DASH__ALGORITHM__FIND__IF_NOT_H__
