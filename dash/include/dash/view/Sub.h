#ifndef DASH__VIEW__SUB_H__INCLUDED
#define DASH__VIEW__SUB_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>
#include <dash/view/Utility.h>


namespace dash {

// -------------------------------------------------------------------------
// View Modifiers (not coupled with origin memory / index space):
// -------------------------------------------------------------------------

#if 0
/**
 * Sub-section, view dimensions maintain domain dimensions.
 *
 * \concept{DashViewConcept}
 */
template <
  dim_t SubDim   = 0,
  dim_t NViewDim,
  class OffsetFirstT,
  class OffsetFinalT >
constexpr ViewSubMod<ViewOrigin<NViewDim>, SubDim>
sub(OffsetFirstT begin,
    OffsetFinalT end) {
  return ViewSubMod<ViewOrigin<NViewDim>, SubDim>(begin, end);
}

/**
 * Sub-section, view dimensions maintain domain dimensions.
 *
 * \concept{DashViewConcept}
 */
template <
  dim_t SubDim   = 0,
  dim_t NViewDim,
  class IndexRangeT >
constexpr ViewSubMod<ViewOrigin<NViewDim>, SubDim>
sub(const IndexRangeT & range) {
  return sub<SubDim>(dash::begin(range),
                     dash::end(range));
}
#endif

#if 0
/**
 * Sub-space projection, view reduces domain by one dimension.
 *
 * \concept{DashViewConcept}
 */
template <
  dim_t SubDim = 0,
  class OffsetT >
constexpr ViewSubMod<ViewOrigin, SubDim>
sub(
    OffsetT offset) {
  return ViewSubMod<ViewOrigin, SubDim>(offset);
}
#endif

// -------------------------------------------------------------------------
// View Proxies (coupled with origin memory / index space):
// -------------------------------------------------------------------------

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainValueT = typename std::decay<DomainT>::type >
constexpr
ViewSubMod<
  DomainValueT,
  SubDim,
  dash::view_traits<DomainValueT>::rank::value >
sub(OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain) {
  return ViewSubMod<
           DomainValueT,
           SubDim,
           dash::view_traits<DomainValueT>::rank::value
         >(std::forward<DomainT>(domain),
           begin,
           end);
}

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetT,
  typename std::enable_if<
             (!std::is_integral<DomainT>::value &&
               std::is_integral<OffsetT>::value ),
             int
           >::type = 0 >
constexpr auto
sub(OffsetT    offset,
    DomainT && domain)
    -> decltype(dash::sub<SubDim>(offset, offset+1, domain)) {
  return dash::sub<SubDim>(offset, offset+1, domain);
}

template <
  dim_t    SubDim  = 0,
  class    OffsetT0,
  class    OffsetT1,
  typename std::enable_if<
             ( std::is_integral<OffsetT0>::value &&
               std::is_integral<OffsetT1>::value ),
             int
           >::type = 0 >
static inline auto sub(OffsetT0 a, OffsetT1 b) {
  return dash::make_pipeable(
           [=](auto && x) {
             return sub<SubDim>(a,b, std::forward<decltype(x)>(x));
           });
}

} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
