#ifndef DASH__VIEW__ORIGIN_H__INCLUDED
#define DASH__VIEW__ORIGIN_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>


namespace dash {

// ------------------------------------------------------------------------
// dash::origin(View)

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
inline typename std::enable_if<
  dash::view_traits<ViewT>::is_view::value,
  const typename dash::view_traits<ViewT>::origin_type &
>::type
origin(const ViewT & view) {
  return view.origin();
}

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
inline typename std::enable_if<
  dash::view_traits<ViewT>::is_view::value,
  typename dash::view_traits<ViewT>::origin_type &
>::type
origin(ViewT & view) {
  return view.origin();
}

// ------------------------------------------------------------------------
// dash::origin(Container)

/**
 *
 * \concept{DashViewConcept}
 */
template <class ContainerT>
constexpr typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
  const ContainerT &
>::type
origin(const ContainerT & container) {
  return container;
}

/**
 *
 * \concept{DashViewConcept}
 */
template <class ContainerT>
inline typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
  ContainerT &
>::type
origin(ContainerT & container) {
  return container;
}

} // namespace dash

#endif // DASH__VIEW__ORIGIN_H__INCLUDED
