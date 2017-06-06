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
 * Variables used in the following:
 *
 * - \c r instance of a range model type
 * - \c o index type, representing element offsets in the range and their
 *        distance
 * - \c i iterator referencing elements in the range
 *
 * \par Types
 *
 * \par Expressions
 *
 * Expression               | Returns | Effect | Precondition | Postcondition
 * ------------------------ | ------- | ------ | ------------ | -------------
 * <tt>*dash::begin(r)</tt> |         |        |              | 
 * <tt>r[o]</tt>            |         |        |              | 
 *
 * \par Functions
 *
 * - \c dash::begin
 * - \c dash::end
 * - \c dash::distance
 * - \c dash::size
 *
 * \par Metafunctions
 *
 * - \c dash::is_range<X>
 *
 * \}
 */


// Related: boost::range
//
// https://github.com/boostorg/range/tree/develop/include/boost/range
//


#include <dash/Types.h>
#include <dash/Meta.h>

#include <type_traits>


namespace dash {

namespace detail {

template<typename T>
struct _is_range_type
{
private:
  typedef char yes;
  typedef long no;

  typedef typename std::decay<T>::type ValueT;

  // Test if x.begin() is valid expression and type x::iterator is
  // defined:
  template<typename C, typename C::iterator (C::*)() = &C::begin >
  static yes has_begin(C *);
  static no  has_begin(...);

  template<typename C, typename C::iterator (C::*)() const = &C::begin >
  static yes has_const_begin(C *);
  static no  has_const_begin(...);

  // Test if x.end() is valid expression and type x::iterator is
  // defined:
  template<typename C, typename C::iterator (C::*)() = &C::end >
  static yes has_end(C *);
  static no  has_end(...);

  template<typename C, typename C::iterator (C::*)() const = &C::end >
  static yes has_const_end(C *);
  static no  has_const_end(...);

public:
  enum { value = (
           (    sizeof(has_begin(static_cast<ValueT*>(nullptr)))
                 == sizeof(yes)
             || sizeof(has_const_begin(static_cast<ValueT*>(nullptr)))
                 == sizeof(yes) )
           &&
           (    sizeof(has_end(static_cast<ValueT*>(nullptr)))
                 == sizeof(yes)
             || sizeof(has_const_end(static_cast<ValueT*>(nullptr)))
                 == sizeof(yes) )
         ) };
};

} // namespace detail

/**
 * Definition of type trait \c dash::is_range<T>
 * with static member \c value indicating whether type \c T is a model
 * of the Range concept.
 *
 * Implemented as test if `dash::begin<T>` and `dash::end<T>` are defined.
 *
 * In the current implementation, range types must specify the
 * return type of `dash::begin<T>` and `dash::end<T>` as type
 * definition `iterator`.
 *
 * This requirement will become obsolete in the near future.
 *
 *
 * Example:
 *:
 * \code
 *   bool g_array_is_range = dash::is_range<
 *                                    dash::Array<int>
 *                                 >::value;
 *   // -> true
 *
 *   bool l_array_is_range = dash::is_range<
 *                                    typename dash::Array<int>::local_type
 *                                 >::value;
 *   // -> true
 *
 *   struct inf_range { 
 *     typedef int           * iterator;
 *     typedef std::nullptr_t  sentinel;
 *
 *     iterator begin() { ... }
 *     sentinel end()   { ... }
 *   };
 *
 *   bool inf_range_is_range = dash::is_range<inf_range>::value;
 *   // -> false
 *   //    because of missing definition
 *   //      iterator dash::end<inf_range> -> iterator
 *
 *   Currently requires specialization as workaround:
 *   template <>
 *   struct is_range<inf_range> : std::integral_value<bool, true> { };
 * \endcode
 */
template <class RangeType>
struct is_range : dash::detail::_is_range_type<
                    typename std::decay<RangeType>::type
                  >
{ };

/**
 * \concept{DashRangeConcept}
 */
template <typename RangeType>
constexpr auto begin(RangeType && range)
  -> decltype(std::forward<RangeType>(range).begin()) {
  return std::forward<RangeType>(range).begin();
}

/**
 * \concept{DashRangeConcept}
 */
template <typename RangeType>
constexpr auto begin(const RangeType & range)
  -> decltype(range.begin()) {
  return range.begin();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto end(RangeType && range)
  -> decltype(std::forward<RangeType>(range).end()) {
  return std::forward<RangeType>(range).end();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto end(const RangeType & range)
  -> decltype(range.end()) {
  return range.end();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto
size(RangeType && r)
  -> decltype(std::forward<RangeType>(r).size()) {
  return std::forward<RangeType>(r).size();
}

/**
 * \concept{DashRangeConcept}
 */
template <class RangeType>
constexpr auto
size(const RangeType & r)
  -> decltype(r.size()) {
  return r.size();
}

} // namespace dash

#endif // DASH__RANGES_H__INCLUDED
