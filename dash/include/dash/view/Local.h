#ifndef DASH__VIEW__LOCAL_H__INCLUDED
#define DASH__VIEW__LOCAL_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>


namespace dash {

template <class ViewType>
constexpr
typename std::enable_if<
  dash::view_traits<ViewType>::is_view::value,
  typename ViewType::local_type &
>::type
local(ViewType & origin) {
  return origin.local();
}

template <class ContainerType>
constexpr
typename std::enable_if<
  !dash::view_traits<ContainerType>::is_view::value,
  typename ContainerType::local_type &
>::type
local(ContainerType & origin) {
  return origin.local;
}

} // namespace dash

#endif // DASH__VIEW__LOCAL_H__INCLUDED
