#ifndef DASH__VIEW__DOMAIN_H__INCLUDED
#define DASH__VIEW__DOMAIN_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>


namespace dash {

// ------------------------------------------------------------------------
// dash::domain(View)

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
constexpr typename std::enable_if<
  dash::view_traits<ViewT>::is_view::value,
  const typename dash::view_traits<ViewT>::domain_type &
>::type
domain(const ViewT & view) {
  return view.domain();
}

// ------------------------------------------------------------------------
// dash::domain(Container)

/**
 *
 * \concept{DashViewConcept}
 */
template <class ContainerT>
constexpr typename std::enable_if<
  !dash::view_traits<ContainerT>::is_view::value,
  const ContainerT &
>::type
domain(const ContainerT & container) {
  return container;
}

} // namespace dash

#endif // DASH__VIEW__DOMAIN_H__INCLUDED
