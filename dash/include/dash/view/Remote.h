#ifndef DASH__VIEW__REMOTE_H__INCLUDED
#define DASH__VIEW__REMOTE_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>


namespace dash {

/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr auto
remote(dash::team_unit_t unit, const ViewType & v)
-> typename std::enable_if<
     dash::view_traits<ViewType>::is_view::value,
     decltype(v.local())
//   const typename ViewType::local_type
   >::type {
  return v.local();
}

/**
 * \concept{DashViewConcept}
 */
template <class ContainerType>
constexpr
typename std::enable_if<
  !dash::view_traits<ContainerType>::is_view::value,
  const typename ContainerType::local_type &
>::type
remote(dash::team_unit_t unit, const ContainerType & c) {
  return c.local;
}


} // namespace dash

#endif // DASH__VIEW__REMOTE_H__INCLUDED
