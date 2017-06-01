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

#include <dash/view/Domain.h>

#include <type_traits>
#include <sstream>


namespace dash {

// -----------------------------------------------------------------------
// Forward-declarations

template <typename ViewT>
struct view_traits;

template <class IndexSetType, class DomainType, std::size_t NDim>
class IndexSetBase;

template <class DomainType>
class IndexSetIdentity;

template <class DomainType>
class IndexSetLocal;

template <class DomainType>
class IndexSetGlobal;

template <class DomainType, std::size_t SubDim>
class IndexSetSub;

template <class RangeOrigin>
class IteratorRange;

template <class Iterator, class Sentinel>
class IteratorRangeOrigin;

template <typename Iterator, typename Sentinel>
class IteratorRangeOrigin<Iterator *, Sentinel *>;

template <
  class ViewModType,
  class DomainType,
  dim_t NDim >
class ViewModBase;

template <
  class DomainType,
  dim_t NDim >
class ViewLocalMod;

template <
  class DomainType,
  dim_t SubDim,
  dim_t NDim >
class ViewSubMod;

template <
  class DomainType,
  dim_t NDim >
class ViewGlobalMod;

// -----------------------------------------------------------------------


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

#if 0

template <
  class RangeOrigin,
  typename std::enable_if<
             view_traits<RangeOrigin>::is_local::value
           > * = 0>
struct view_traits<IteratorRange<RangeOrigin> >
{
private:
  typedef IteratorRange<RangeOrigin> RangeT;
public:
  typedef RangeT                                               domain_type;
  typedef RangeT                                               origin_type;
  typedef RangeT                                                image_type;
  typedef RangeT                                               global_type;
  typedef RangeT                                                local_type;
  typedef typename RangeT::index_type                           index_type;
  typedef typename RangeT::size_type                             size_type;
  typedef dash::IndexSetIdentity<RangeT>                    index_set_type;

  /// Whether the view type is a projection (has less dimensions than the
  /// view's domain type).
  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, false>                is_view;
  /// Whether the view is the origin domain.
  typedef std::integral_constant<bool, false>                is_origin;
  /// Whether the view / container type is a local view.
  /// \note A container type is local if it is identical to its
  ///       \c local_type
  typedef std::integral_constant<bool, true>                 is_local;

  typedef std::integral_constant<dim_t, 1>                   rank;
};

/**
 * Specialization of adapter template for range concept, wraps `begin`
 * and `end` pointers in range type.
 */
template <
  typename LocalIterator,
  typename LocalSentinel >
class IteratorRange<LocalIterator *, LocalSentinel *>
{
  typedef IteratorRange<LocalIterator *, LocalSentinel *> self_t;

  LocalIterator * _begin;
  LocalSentinel * _end;

public:
  typedef LocalIterator *                                         iterator;
  typedef LocalSentinel *                                         sentinel;
  typedef const LocalIterator *                             const_iterator;
  typedef const LocalSentinel *                             const_sentinel;

  typedef dash::default_index_t                                 index_type;
  typedef dash::default_size_t                                   size_type;

  typedef LocalIterator                                         value_type;

  typedef iterator local_iterator;
  typedef sentinel local_sentinel;
            
  typedef IteratorRange<local_iterator, local_sentinel>         local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, true>                      is_local;

public:
  constexpr IteratorRange(iterator begin, sentinel end)
  : _begin(begin)
  , _end(end)
  { }

  constexpr const_iterator begin() const { return _begin; }
  constexpr const_sentinel end()   const { return _end;   }

  iterator begin() { return _begin; }
  sentinel end()   { return _end;   }

  constexpr size_type size() const { return std::distance(_begin, _end); }

  constexpr const local_type & local() const {
    return *this;
  }
};

#endif

} // namespace dash

#endif // DASH__RANGES_H__INCLUDED
