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
template <class ContainerT>
typename dash::view_traits<ContainerT>::origin_type
origin(const ContainerT & container);

#else

template <class ContainerT>
constexpr typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
  const typename dash::view_traits<ContainerT>::origin_type &
>::type
origin(const ContainerT & container) {
  return container;
}

template <class ContainerT>
typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
  typename dash::view_traits<ContainerT>::origin_type &
>::type
origin(ContainerT & container) {
  return container;
}

template <class ViewT>
constexpr auto
origin(const ViewT & view)
  -> typename std::enable_if<
       dash::view_traits<ViewT>::is_view::value,
       const typename dash::view_traits<ViewT>::origin_type &
    // const decltype(dash::origin(view.domain()))
     >::type {
  // recurse upwards:
  return dash::origin(view.domain());
}

#if 0
template <class ViewT>
auto
origin(ViewT & view)
  -> typename std::enable_if<
       dash::view_traits<ViewT>::is_view::value,
       typename dash::view_traits<ViewT>::origin_type &
    // decltype(dash::origin(view.domain()))
     >::type {
  // recurse upwards:
  return dash::origin(view.domain());
}
#endif

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__ORIGIN_H__INCLUDED
