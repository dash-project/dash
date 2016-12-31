#ifndef DASH__RANGES_H__INCLUDED
#define DASH__RANGES_H__INCLUDED

/**
 * \defgroup  DashRangeConcept  Multidimensional Range Concept
 *
 * \ingroup DashNDimConcepts
 * \{
 * \par Description
 *
 * Definitions for multidimensional range expressions.
 *
 * \see DashDimensionalConcept
 * \see DashViewConcept
 * \see DashIteratorConcept
 * \see \c dash::view_traits
 *
 * \par Expressions
 *
 * - \c dash::begin
 * - \c dash::end
 *
 * \}
 */


#include <dash/Dimensional.h>
#include <type_traits>

namespace dash {

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto begin(RangeType & range) -> decltype(range.begin()) {
  return range.begin();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto end(RangeType & range) -> decltype(range.end()) {
  return range.end();
}

} // namespace dash

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/LocalRanges.h>

#endif // DASH__RANGES_H__INCLUDED
