#ifndef DASH__VIEW__CHUNKED_H__INCLUDED
#define DASH__VIEW__CHUNKED_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/Domain.h>
#include <dash/view/Local.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/SetIntersect.h>


namespace dash {

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

template <
  class DomainType >
class ViewBlockMod;

#if 0
template <
  class ContainerType,
  class OffsetT >
constexpr auto
block(
  OffsetT               block_idx,
  const ContainerType & container)
-> typename std::enable_if<
     !dash::view_traits<ContainerType>::is_view::value,
     decltype(container.block(0))
   >::type {
  return container.block(block_idx);
}
#endif

/**
 * Blocks view from global view
 *
 */
template <
  class ViewType,
  class OffsetT >
constexpr auto
block(
  OffsetT    block_idx,
  const ViewType & view)
-> typename std::enable_if<
     (//  dash::view_traits<ViewType>::is_view::value &&
         !dash::view_traits<ViewType>::is_local::value   ),
     ViewBlockMod<ViewType>
   >::type {
  return ViewBlockMod<ViewType>(view, block_idx);
}

/**
 * Blocks view from local view
 *
 */
template <
  class ViewType,
  class OffsetT >
constexpr auto
block(
  OffsetT          block_idx,
  const ViewType & view)
-> typename std::enable_if<
     (// dash::view_traits<ViewType>::is_view::value &&
         dash::view_traits<ViewType>::is_local::value   ),
     decltype(dash::block(block_idx, dash::local(dash::origin(view))))
   >::type {
  return dash::local(dash::origin(view)).block(block_idx);
}

} // namespace dash

#endif // DASH__VIEW__CHUNKED_H__INCLUDED
