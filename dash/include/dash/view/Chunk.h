#ifndef DASH__VIEW__CHUNK_H__INCLUDED
#define DASH__VIEW__CHUNK_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/SetIntersect.h>
#include <dash/view/Domain.h>
#include <dash/view/Local.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewChunksMod.h>


namespace dash {

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

/**
 * Chunked view from global view.
 *
 */
template <
  class ViewType,
  class OffsetT >
constexpr auto
chunks(
  OffsetT          block_idx,
  const ViewType & view)
    -> typename std::enable_if<
         (//  dash::view_traits<ViewType>::is_view::value &&
             !dash::view_traits<ViewType>::is_local::value   ),
         ViewChunksMod<ViewType>
       >::type {
  return ViewChunksMod<ViewType>(view, block_idx);
}

} // namespace dash

#endif // DASH__VIEW__CHUNK_H__INCLUDED
