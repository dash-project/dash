#ifndef DASH__VIEW__BLOCK_H__INCLUDED
#define DASH__VIEW__BLOCK_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>
#include <dash/view/ViewMod.h>


namespace dash {

#if 0
/**
 *
 * \concept{DashViewConcept}
 */
template <
  class ViewT,
  class BlockIndexT >
constexpr typename std::enable_if<
  dash::view_traits<ViewT>::is_view::value,
  dash::ViewBlockMod<ViewT>
>::type
block(BlockIndexT block_index, const ViewT & view) {
  return ViewBlockMod<ViewT>(view, block_index);
}
#endif

}

#endif // DASH__VIEW__BLOCK_H__INCLUDED
