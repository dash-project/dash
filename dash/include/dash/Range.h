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
 * \par Functions
 *
 * - \c dash::begin
 * - \c dash::end
 *
 * \par Metafunctions
 *
 * - \c dash::is_range<X>
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

/**
 * Type trait for testing if `dash::begin<T>` and `dash::end<T>`
 * are defined.
 *
 * In the current implementation, range types must specify the
 * return type of `dash::begin<T>` and `dash::end<T>` as type
 * definition `iterator`.
 *
 * This requirement will become obsolete in the near future.
 *
 *
 * Example:
 *
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
struct is_range : dash::detail::_is_range_type<RangeType> { };

template <class RangeType, class Iterator, class Sentinel = Iterator>
class RangeBase {
public:
  typedef Iterator iterator;
  typedef Sentinel sentinel;
  typedef typename Iterator::index_type index_type;
protected:
  RangeType & derived() {
    return static_cast<RangeType &>(*this);
  }
  const RangeType & derived() const {
    return static_cast<const RangeType &>(*this);
  }
};

template <class Iterator, class Sentinel = Iterator>
class IteratorRange
: public RangeBase< IteratorRange<Iterator, Sentinel>,
                    Iterator, Sentinel >
{
  Iterator _begin;
  Sentinel _end;

public:
  template <class Container>
  constexpr explicit IteratorRange(Container && c)
  : _begin(c.begin()), _end(c.end()) { }

  constexpr IteratorRange(Iterator begin, Sentinel end)
  : _begin(std::move(begin)), _end(std::move(end)) { }

  constexpr Iterator begin() const { return _begin; }
  constexpr Iterator end()   const { return _end;   }
};

template <class Iterator, class Sentinel>
constexpr dash::IteratorRange<Iterator, Sentinel>
make_range(Iterator begin, Sentinel end) {
  return dash::IteratorRange<Iterator, Sentinel>(
           std::move(begin),
           std::move(end));
}

} // namespace dash

#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/LocalRanges.h>

#endif // DASH__RANGES_H__INCLUDED
