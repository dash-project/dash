#ifndef DASH__VIEW__CHUNKED_H__INCLUDED
#define DASH__VIEW__CHUNKED_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>



template <
  class DomainType >
class ViewBlocksMod;



namespace dash {

template <class ViewType>
constexpr auto
blocks(const ViewType & domain)
-> typename std::enable_if<
     dash::view_traits<ViewType>::is_view::value,
     const ViewBlocksMod<ViewType> &
   >::type {
  return ViewBlocksMod<ViewType>(domain);
}

template <
  class ContainerType,
  class OffsetT >
constexpr auto
block(
  OffsetT               block_idx,
  const ContainerType & container)
-> typename std::enable_if<
     !dash::view_traits<ContainerType>::is_view::value,
     const decltype(container.block(0)) &
   >::type {
  return container.block(block_idx);
}

template <
  class ViewType,
  class OffsetT >
constexpr auto
block(
  OffsetT          block_idx,
  const ViewType & domain)
-> typename std::enable_if<
     dash::view_traits<ViewType>::is_view::value,
     const decltype(dash::block(0, domain)) &
   >::type {
  return domain.block(block_idx);
}

} // namespace dash

#endif // DASH__VIEW__CHUNKED_H__INCLUDED
