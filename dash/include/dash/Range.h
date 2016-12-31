#ifndef DASH__RANGES_H__INCLUDED
#define DASH__RANGES_H__INCLUDED

#include <dash/Dimensional.h>

#include <type_traits>

/**
 * \defgroup  DashRangeConcept  Multi-dimensional Range Concept
 *
 * \see DashDimensionalConcept
 * \see DashViewConcept
 * \see DashIteratorConcept
 *
 * \see \c dash::view_traits
 *
 * \ingroup DashNDimConcepts
 * \{
 * \par Description
 *
 * \par Expressions
 *
 * - \c dash::begin
 * - \c dash::end
 *
 * \}
 */

namespace dash {

template <class RangeType>
constexpr auto begin(RangeType & range) -> decltype(range.begin()) {
  return range.begin();
}

template <class RangeType>
constexpr auto end(RangeType & range) -> decltype(range.end()) {
  return range.end();
}

} // namespace dash

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/LocalRanges.h>

#endif // DASH__RANGES_H__INCLUDED
