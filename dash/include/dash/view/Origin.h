#ifndef DASH__VIEW__ORIGIN_H__INCLUDED
#define DASH__VIEW__ORIGIN_H__INCLUDED


#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/Domain.h>
#include <dash/view/ViewTraits.h>


namespace dash {

#ifdef DOXYGEN

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
typename dash::view_traits<ViewT>::origin_type
origin(const ViewT & view);

#else

template <class ContainerT>
constexpr typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
//const typename dash::view_traits<ContainerT>::origin_type &
  const ContainerT &
>::type
origin(const ContainerT & container) {
  return container;
}

template <class ContainerT>
typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
//typename dash::view_traits<ContainerT>::origin_type &
  ContainerT &
>::type
origin(ContainerT & container) {
  return container;
}

template <class ViewT>
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       dash::view_traits<ViewT>::is_view::value
       && !dash::view_traits<
            typename dash::view_traits<ViewT>::domain_type
          >::is_local::value,
       const typename dash::view_traits<ViewT>::origin_type &
//     const decltype(dash::origin(view.domain()))
     >::type {
  // recurse upwards:
  return dash::origin(view.domain());
}

template <class ViewT>
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       dash::view_traits<ViewT>::is_view::value
       && dash::view_traits<
            typename dash::view_traits<ViewT>::domain_type
          >::is_local::value,
       const typename dash::view_traits<ViewT>::domain_type &
     >::type {
  // recurse upwards:
  return view.domain();
}

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__ORIGIN_H__INCLUDED
