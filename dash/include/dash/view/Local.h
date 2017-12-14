#ifndef DASH__VIEW__LOCAL_H__INCLUDED
#define DASH__VIEW__LOCAL_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>


namespace dash {

namespace detail {
  /**
   * Definition of type trait \c dash::detail::has_type_local_type<T>
   * with static member \c value indicating whether type \c T provides
   * dependent type \c local_type.
   */
  DASH__META__DEFINE_TRAIT__HAS_TYPE(local_type)
}

/**
 * \concept{DashViewConcept}
 */
template <
  class    ViewType,
  typename ViewValueT = typename std::decay<ViewType>::type >
constexpr auto
local(ViewType & v)
-> typename std::enable_if<
     ( std::is_pointer< typename ViewType::iterator >::value ||
       ( // dash::view_traits<ViewValueT>::is_origin::value &&
         dash::view_traits<ViewValueT>::is_local::value ) ),
     ViewType &
   >::type {
  return v;
}

#if 0
template <
  class    ContainerLocalType,
  typename ContainerLocalDecayType
             = typename std::decay<ContainerLocalType>::type >
constexpr
typename std::enable_if<
  ( dash::view_traits<ContainerLocalDecayType>::is_origin::value &&
    dash::view_traits<ContainerLocalDecayType>::is_local::value ),
  ContainerLocalType &
>::type
local(ContainerLocalType & cl) {
  return cl;
}
#endif

template <
  class    ContainerType,
  typename ContainerDecayType
             = typename std::decay<ContainerType>::type >
constexpr
typename std::enable_if<
  ( !dash::view_traits<ContainerDecayType>::is_view::value &&
    !dash::view_traits<ContainerDecayType>::is_local::value &&
     dash::view_traits<ContainerDecayType>::is_origin::value ),
  const typename ContainerType::local_type &
>::type
local(const ContainerType & c) {
  return c.local;
}

#if 0
/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr auto
local(const ViewType & v)
-> typename std::enable_if<
     !dash::view_traits<ViewType>::is_view::value &&
     !dash::view_traits<ViewType>::is_local::value &&
     dash::detail::has_type_local_type<ViewType>::value,
     dash::IndexSetIdentity<const typename ViewType::local_type>
   >::type {
  return IndexSetIdentity<const typename ViewType::local_type>(
           v.local());
}
#endif

/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr auto
local(const ViewType & v)
-> typename std::enable_if<
     dash::view_traits<ViewType>::is_view::value,
     decltype(v.local())
   >::type {
  return v.local();
}

template <
  class    ViewType,
  typename ViewValueT = typename std::decay<ViewType>::type >
constexpr auto
local(ViewType && v)
-> typename std::enable_if<
    ( dash::view_traits<ViewValueT>::is_view::value &&
     !dash::view_traits<ViewValueT>::is_local::value ),
     decltype(std::forward<ViewType>(v).local())
   >::type {
 return std::forward<ViewType>(v).local();
}

#if 0
template <class ContainerType>
constexpr auto local(const ContainerType & c)
  -> typename std::enable_if<
         !dash::view_traits< ContainerType >::is_view::value
       && dash::view_traits< ContainerType  >::rank::value == 1,
       dash::ViewSubMod<ContainerType, 0>
     >::type {
  return dash::ViewSubMod<ContainerType, 0>(0, c.size(), c);
}

template <class ContainerType>
constexpr auto local(const ContainerType & c)
  -> typename std::enable_if<
         !dash::view_traits< ContainerType >::is_view::value
       && (dash::view_traits< ContainerType  >::rank::value > 1),
       dash::NViewSubMod<ContainerType, 0>
     >::type {
  return dash::NViewSubMod<ContainerType, 0>(0, c.extents()[0], c);
}
#endif

/**
 * Convert global iterator referencing an element the active unit's
 * memory to a corresponding native pointer referencing the element.
 *
 * Precondition:  \c g_it  is local
 *
 */
template <class GlobalIterator>
constexpr auto local(
  /// Global iterator referencing element in local memory
  const GlobalIterator & g_it)
->  decltype((g_it - g_it.pos()).local()) {
  return g_it.local();
}

// =========================================================================
// Multidimensional Views
// =========================================================================

} // namespace dash

#endif // DASH__VIEW__LOCAL_H__INCLUDED
