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
constexpr auto begin(const RangeType & range) -> decltype(range.begin()) {
  return range.begin();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto end(const RangeType & range) -> decltype(range.end()) {
  return range.end();
}

#if 0
template <class X>
bool begin(X x) { return false; }

template <class X>
bool end(X x) { return false; }
#endif

namespace detail {

template<typename T>
struct _is_range_type
{
private:
  typedef char yes;
  typedef long no;

#if 0
  template <typename C> static yes test_beg(
                                     decltype(
                                       dash::begin(
                                         std::move(std::declval<T>())
                                       )
                                     ) * );
  template <typename C> static no  test_beg(...);    

  template <typename C> static yes test_end(
                                     decltype(
                                       dash::end(
                                         std::move(std::declval<T>())
                                       )
                                     ) * );
  template <typename C> static no  test_end(...);    

public:
  enum { value = (    sizeof(test_beg<T>(nullptr)) == sizeof(yes)
                   && sizeof(test_end<T>(nullptr)) == sizeof(yes) ) };
#else
  template<typename C, typename C::iterator (C::*)() = &C::begin >
  static yes has_begin(C *);

//template<typename C, typename begin_decl =
//                                   decltype(
//                                     dash::begin(
//                                       std::move(std::declval<T>())
//                                     )) >
//static yes has_begin(C *);
  
  static no  has_begin(...);

public:
  enum { value = (    sizeof(has_begin(static_cast<T*>(nullptr)))
                   == sizeof(yes) ) };
#endif
};

} // namespace detail

template <class RangeType>
struct is_range : dash::detail::_is_range_type<RangeType> { };

} // namespace dash

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/LocalRanges.h>

#endif // DASH__RANGES_H__INCLUDED
