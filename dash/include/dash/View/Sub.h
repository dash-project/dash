#ifndef DASH__VIEW__SUB_H__INCLUDED
#define DASH__VIEW__SUB_H__INCLUDED

#include <dash/Types.h>
#include <dash/View/ViewMod.h>


namespace dash {

// Sub-space slice, view dimensions maintain origin dimensions

template <
  dim_t SubDim,
  class OffsetT
>
ViewMod<0, OffsetT>
sub(OffsetT begin, OffsetT end) {
  ViewMod<0, OffsetT> sub_viewmod;
  return sub_viewmod.sub<SubDim>(begin, end);
}

template <
  dim_t SubDim,
  class IndexRangeT
>
ViewMod<0, typename IndexRangeT::index_type>
sub(IndexRangeT range) {
  return sub<SubDim>(dash::begin(range),
                     dash::end(range));
}

// Sub-space projection, view reduces origin domain by one dimension

template <
  dim_t SubDim,
  class OffsetT
>
ViewMod<-1, OffsetT> sub(OffsetT offset) {
  ViewMod<-1, OffsetT> sub_viewmod;
  return sub_viewmod.sub<SubDim>(offset);
}


} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
