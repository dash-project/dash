#ifndef DASH__VIEW__GLOBAL_H__INCLUDED
#define DASH__VIEW__GLOBAL_H__INCLUDED

#include <dash/view/Utility.h>
#include <dash/view/ViewTraits.h>


namespace dash {

/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr
typename std::enable_if<
  dash::view_traits<ViewType>::is_view::value &&
  dash::view_traits<ViewType>::is_local::value,
  const typename ViewType::global_type &
>::type
global(const ViewType & v) {
  return v.global();
}

/**
 * \concept{DashViewConcept}
 */
template <class ContainerType>
constexpr
typename std::enable_if<
  !dash::view_traits<ContainerType>::is_view::value ||
  !dash::view_traits<ContainerType>::is_local::value,
  ContainerType &
>::type
global(ContainerType & c) {
  return c;
}

static inline auto global() {
  return dash::make_pipeable(
           [](auto && x) {
             return global(std::forward<decltype(x)>(x));
           });
}

} // namespace dash

#endif // DASH__VIEW__GLOBAL_H__INCLUDED
