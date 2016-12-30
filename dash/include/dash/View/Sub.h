#ifndef DASH__VIEW__SUB_H__INCLUDED
#define DASH__VIEW__SUB_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/View/ViewMod.h>


namespace dash {

// Sub-space slice, view dimensions maintain origin dimensions

template <
  dim_t SubDim = 0,
  class OffsetT >
ViewSubMod<0, OffsetT>
sub(OffsetT begin, OffsetT end) {
  return ViewSubMod<0, OffsetT>(begin, end);
}

template <
  dim_t SubDim = 0,
  class IndexRangeT >
ViewSubMod<0, typename IndexRangeT::index_type>
sub(IndexRangeT range) {
  return sub<SubDim>(dash::begin(range),
                     dash::end(range));
}

// Sub-space projection, view reduces origin domain by one dimension

template <
  dim_t SubDim = 0,
  class OffsetT >
ViewSubMod<-1, OffsetT>
sub(OffsetT offset) {
 return ViewSubMod<-1, OffsetT>(offset);
}


} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
