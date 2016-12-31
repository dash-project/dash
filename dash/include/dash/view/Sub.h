#ifndef DASH__VIEW__SUB_H__INCLUDED
#define DASH__VIEW__SUB_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

// -------------------------------------------------------------------------
// View Modifiers (not coupled with origin memory / index space):
// -------------------------------------------------------------------------

// Sub-space slice, view dimensions maintain origin dimensions

template <
  dim_t SubDim = 0,
  class OffsetT >
ViewSubMod<0, OffsetT>
constexpr sub(
    OffsetT begin,
    OffsetT end) {
  return ViewSubMod<0, OffsetT>(begin, end);
}

template <
  dim_t SubDim = 0,
  class IndexRangeT >
ViewSubMod<0, typename IndexRangeT::index_type>
constexpr sub(
    IndexRangeT range) {
  return sub<SubDim>(dash::begin(range),
                     dash::end(range));
}

// Sub-space projection, view reduces origin domain by one dimension

template <
  dim_t SubDim = 0,
  class OffsetT >
ViewSubMod<-1, OffsetT>
constexpr sub(
    OffsetT offset) {
  return ViewSubMod<-1, OffsetT>(offset);
}

// -------------------------------------------------------------------------
// View Proxies (coupled with origin memory / index space):
// -------------------------------------------------------------------------

// Sub-space slice, view dimensions maintain origin dimensions

template <
  dim_t SubDim  = 0,
  class RangeT,
  class OffsetT = typename RangeT::index_type >
ViewSubMod<0, RangeT, OffsetT>
constexpr sub(
    OffsetT   begin,
    OffsetT   end,
    RangeT  & origin) {
  return ViewSubMod<0, RangeT, OffsetT>(
           origin,
           begin,
           end);
}

// Sub-space projection, view reduces origin domain by one dimension

} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
