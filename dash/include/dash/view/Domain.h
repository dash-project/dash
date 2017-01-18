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
constexpr auto
domain(const ViewT & view)
  -> typename std::enable_if<
       dash::view_traits<ViewT>::is_view::value,
       const typename dash::view_traits<ViewT>::domain_type &
    // decltype(view.domain())
     >::type {
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
