#ifndef DASH__VIEW__BLOCK_H__INCLUDED
#define DASH__VIEW__BLOCK_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>
#include <dash/view/ViewMod.h>
#include <dash/view/ViewBlocksMod.h>


namespace dash {

/**
 *
 * \concept{DashViewConcept}
 */
template <
  class    ViewT,
  class    BlockIndexT,
  dim_t    NDim
             = dash::view_traits<ViewT>::rank::value,
  typename std::enable_if<
             dash::view_traits<ViewT>::is_view::value,
             int >::type = 0
>
constexpr auto
block(BlockIndexT block_index, const ViewT & view) {
  return ViewBlockMod<ViewT, NDim>(view, block_index);
}

template <
  class    ViewT,
  class    BlockIndexT,
  dim_t    NDim
             = dash::view_traits<ViewT>::rank::value,
  typename std::enable_if<
             !dash::view_traits<ViewT>::is_view::value,
             int >::type = 0
>
constexpr auto
block(BlockIndexT block_index, const ViewT & view) {
  return dash::blocks(view)[block_index];
}

template <
  class    BlockIndexT,
  typename std::enable_if<
             std::is_integral<
               typename std::decay<BlockIndexT>::type
             >::value,
             int
           >::type = 0 >
static inline auto block(BlockIndexT b) {
  return dash::make_pipeable(
           [=](auto && x) {
             return block(b, std::forward<decltype(x)>(x));
           });
}

}

#endif // DASH__VIEW__BLOCK_H__INCLUDED
