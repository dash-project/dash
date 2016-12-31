#ifndef DASH__RANGES_H__INCLUDED
#define DASH__RANGES_H__INCLUDED

#include <type_traits>


namespace dash {

#ifdef DOXYGEN

/**
 * \defgroup  DashRangeConcept  Multi-dimensional Range Concept
 *
 * \see DashViewConcept
 * \see DashIteratorConcept
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 * 
 * \par Types
 *
 * \par Expressions
 *
 * \}
 */

#endif // DOXYGEN

template <class RangeType>
constexpr auto begin(RangeType & range) -> decltype(range.begin()) {
  return range.begin();
}

template <class RangeType>
constexpr auto end(RangeType & range) -> decltype(range.end()) {
  return range.end();
}

template <class IndexType>
constexpr typename std::enable_if<
  std::is_integral<IndexType>::value, IndexType >::type
index(IndexType idx) {
  return idx;
}

template <class Iterator>
constexpr auto index(Iterator it) -> decltype(it.pos()) {
  return it.pos();
}

} // namespace dash

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/LocalRanges.h>

#endif // DASH__RANGES_H__INCLUDED
