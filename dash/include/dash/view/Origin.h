#ifndef DASH__VIEW__ORIGIN_H__INCLUDED
#define DASH__VIEW__ORIGIN_H__INCLUDED


#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/Domain.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/Local.h>


namespace dash {

#ifdef DOXYGEN

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
typename dash::view_traits<ViewT>::origin_type
origin(ViewT & view);

#else

#if 0
template <class ContainerT>
typename std::enable_if<
   view_traits<typename std::decay<ContainerT>::type>::is_origin::value ||
  !view_traits<typename std::decay<ContainerT>::type>::is_view::value,
  ContainerT
>::type
origin(ContainerT && container) {
  return std::forward<ContainerT>(container);
}
#else
template <
  class    ContainerT,
  typename ContainerValueT = typename std::decay<ContainerT>::type >
typename std::enable_if<
   view_traits<ContainerValueT>::is_origin::value ||
  !view_traits<ContainerValueT>::is_view::value,
  const ContainerT &
>::type
origin(const ContainerT & container) {
  return container;
}
#endif

template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewValueT>::is_view::value &&
        !dash::view_traits<ViewValueT>::is_origin::value &&
        !dash::view_traits<ViewValueT>::is_local::value ),
       const typename dash::view_traits<ViewValueT>::origin_type &
//     decltype(dash::origin(view.domain()))
     >::type {
  // Recurse to origin of global view:
  return dash::origin(view.domain());
}

template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
origin(ViewT && view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewValueT>::is_view::value &&
         std::is_copy_constructible<
           typename dash::view_traits<ViewValueT>::origin_type
         >::value &&
        !dash::view_traits<ViewValueT>::is_origin::value &&
        !dash::view_traits<ViewValueT>::is_local::value ),
       typename dash::view_traits<ViewValueT>::origin_type
//     decltype(dash::origin(std::forward<ViewT>(view).domain()))
     >::type {
  // Recurse to origin of global view:
  return dash::origin(std::forward<ViewT>(view).domain());
}

template <
  class    ContainerT,
  typename ContainerValueT = typename std::decay<ContainerT>::type >
constexpr auto
global_origin(const ContainerT & container)
  -> typename std::enable_if<
       dash::view_traits<ContainerValueT>::is_origin::value,
       const ContainerT &
     >::type {
  return container;
}

template <
  class    ContainerT,
  typename ContainerValueT = typename std::decay<ContainerT>::type >
constexpr auto
global_origin(ContainerT && container)
  -> typename std::enable_if<
       dash::view_traits<ContainerValueT>::is_origin::value,
       decltype(std::forward<ContainerT>(container))
     >::type {
  return std::forward<ContainerT>(container);
}

template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
global_origin(const ViewT & view)
  -> typename std::enable_if<
       !dash::view_traits<ViewValueT>::is_origin::value,
       const typename dash::view_traits<ViewValueT>::origin_type &
     >::type {
  // Recurse to origin of local view:
  return dash::global_origin(view.domain());
}

template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
origin(ViewT && view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewValueT>::is_view::value &&
         dash::view_traits<
           typename dash::view_traits<ViewValueT>::domain_type
         >::is_local::value ),
       decltype(dash::local(
                  dash::global_origin(std::forward<ViewT>(view).domain())))
     >::type {
  // Recurse to origin of local view:
  return dash::local(
           dash::global_origin(
             std::forward<ViewT>(view).domain()));
}

template <
  class    ViewT,
  typename ViewValueT = typename std::decay<ViewT>::type >
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewValueT>::is_view::value &&
         dash::view_traits<ViewValueT>::is_local::value &&
        !dash::view_traits<
           typename dash::view_traits<ViewValueT>::domain_type
         >::is_local::value ),
//     const typename dash::view_traits<ViewValueT>::origin_type &
       decltype(dash::global_origin(view.domain()))
     >::type {
  // Recurse to global origin of local view:
  return dash::global_origin(view.domain());
}

#endif // DOXYGEN

static inline auto origin() {
  return dash::make_pipeable(
           [](auto && x) {
             return origin(std::forward<decltype(x)>(x));
           });
}

} // namespace dash

#endif // DASH__VIEW__ORIGIN_H__INCLUDED
