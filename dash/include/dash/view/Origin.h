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

template <class ContainerT>
typename std::enable_if<
   view_traits<typename std::decay<ContainerT>::type>::is_origin::value ||
  !view_traits<typename std::decay<ContainerT>::type>::is_view::value,
  ContainerT
>::type
origin(ContainerT && container) {
  return std::forward<ContainerT>(container);
}

template <class ViewT>
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewT>::is_view::value &&
        !dash::view_traits<ViewT>::is_origin::value &&
        !dash::view_traits<ViewT>::is_local::value ),
       const typename dash::view_traits<ViewT>::origin_type &
     >::type {
  // Recurse to origin of global view:
  return dash::origin(view.domain());
}


template <class ContainerT>
typename std::enable_if<
  dash::view_traits<ContainerT>::is_origin::value,
  ContainerT &
>::type
global_origin(ContainerT & container) {
  return container;
}

template <class ViewT>
constexpr auto
global_origin(const ViewT & view)
  -> typename std::enable_if<
       !dash::view_traits<ViewT>::is_origin::value,
       const typename dash::view_traits<ViewT>::origin_type &
     >::type {
  // Recurse to origin of local view:
  return dash::global_origin(view.domain());
}

template <class ViewT>
constexpr auto
origin(ViewT && view)
  -> typename std::enable_if<
       ( dash::view_traits<
           typename std::decay<ViewT>::type
         >::is_view::value &&
         dash::view_traits<
           typename dash::view_traits<
             typename std::decay<ViewT>::type
           >::domain_type
         >::is_local::value ),
       const typename dash::view_traits<ViewT>::origin_type::local_type &
    // typename dash::view_traits<
    //            typename std::decay<ViewT>::type
    //          >::origin_type::local_type
     >::type {
  // Recurse to origin of local view:
  return dash::local(
           dash::global_origin(
             std::forward<ViewT>(view).domain()));
}

template <class ViewT>
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       ( dash::view_traits<ViewT>::is_view::value &&
         dash::view_traits<ViewT>::is_local::value &&
        !dash::view_traits<
           typename dash::view_traits<ViewT>::domain_type
         >::is_local::value ),
       const typename dash::view_traits<ViewT>::origin_type &
     >::type {
  // Recurse to global origin of local view:
  return dash::global_origin(view.domain());
}

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__ORIGIN_H__INCLUDED
