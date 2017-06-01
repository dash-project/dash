#ifndef DASH__VIEW__DOMAIN_H__INCLUDED
#define DASH__VIEW__DOMAIN_H__INCLUDED

#include <dash/Types.h>
#include <dash/Meta.h>

namespace dash {

namespace detail {
  /**
   * Definition of type trait \c dash::detail::has_type_domain_type<T>
   * with static member \c value indicating whether type \c T provides
   * dependent type \c domain_type.
   */
  DASH__META__DEFINE_TRAIT__HAS_TYPE(domain_type);
}


// -----------------------------------------------------------------------
// Forward-declarations

template <typename ViewT>
struct view_traits;

// ------------------------------------------------------------------------
// dash::domain(View)

/**
 *
 * \concept{DashViewConcept}
 */
template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
domain(ViewT && view)
  -> typename std::enable_if<
    // dash::view_traits<ViewValueT>::is_view::value,
       dash::detail::has_type_domain_type<ViewValueT>::value,
       decltype(std::forward<ViewT>(view).domain())
     >::type {
  return std::forward<ViewT>(view).domain();
}

template <class ViewT>
constexpr auto
domain(const ViewT & view)
  -> typename std::enable_if<
       dash::detail::has_type_domain_type<ViewT>::value,
    // dash::view_traits<ViewT>::is_view::value,
       decltype(view.domain())
    // const typename dash::view_traits<ViewT>::domain_type &
    // const typename ViewT::domain_type &
     >::type {
  return view.domain();
}

// ------------------------------------------------------------------------
// dash::domain(Container)

/**
 *
 * \concept{DashViewConcept}
 */
template <
  class    ContainerT,
  typename ContainerValueT = typename std::decay<ContainerT>::type >
constexpr typename std::enable_if<
//!dash::view_traits<ContainerValueT>::is_view::value,
  !dash::detail::has_type_domain_type<ContainerValueT>::value,
  ContainerT &
>::type
domain(ContainerT & container) {
  return container;
}

/**
 *
 * \concept{DashViewConcept}
 */
template <
  class    ContainerT,
  typename ContainerValueT = typename std::decay<ContainerT>::type >
constexpr typename std::enable_if<
//!dash::view_traits<ContainerValueT>::is_view::value,
  !dash::detail::has_type_domain_type<ContainerValueT>::value,
  const ContainerT &
>::type
domain(const ContainerT & container) {
  return container;
}

} // namespace dash

#endif // DASH__VIEW__DOMAIN_H__INCLUDED
