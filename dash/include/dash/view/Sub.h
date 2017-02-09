#ifndef DASH__VIEW__SUB_H__INCLUDED
#define DASH__VIEW__SUB_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>
#include <dash/view/NViewMod.h>


namespace dash {

// -------------------------------------------------------------------------
// View Modifiers (not coupled with origin memory / index space):
// -------------------------------------------------------------------------

// Sub-space slice, view dimensions maintain domain dimensions

/**
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

// Sub-space projection, view reduces domain by one dimension

#if 0
/**
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

// Sub-space slice, view dimensions maintain domain dimensions

#if 0
/**
 * \concept{DashViewConcept}
 */
template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainValueT =
             typename std::remove_const<
               typename std::remove_reference<DomainT>::type
             >::type
>
constexpr auto
sub(
    OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT       & domain)
  -> typename std::enable_if<
       dash::view_traits<
         DomainValueT
       >::rank::value == 1,
       ViewSubMod<DomainT, SubDim>
     >::type {
  return ViewSubMod<DomainT, SubDim>(
           domain,
           begin,
           end);
}
#endif

#if 1
template <
  dim_t SubDim  = 0,
  class DomainT,
  class OffsetFirstT,
  class OffsetFinalT,
  typename DomainValueT =
  //         typename std::remove_const<
               typename std::remove_reference<DomainT>::type
  //         >::type
>
constexpr auto
sub(
    OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain)
  -> typename std::enable_if<
       dash::view_traits<
         DomainValueT
       >::rank::value == 1,
       ViewSubMod<DomainValueT, SubDim>
     >::type {
  return ViewSubMod<DomainValueT, SubDim>(
           std::forward<DomainT>(domain),
           begin,
           end);
}
#endif

// =========================================================================
// Multidimensional Views
// =========================================================================

template <
  dim_t SubDim  = 0,
  class DomainT,
  class OffsetFirstT,
  class OffsetFinalT >
constexpr auto
sub(
    OffsetFirstT    begin,
    OffsetFinalT    end,
    const DomainT & domain)
  -> typename std::enable_if<
       (dash::view_traits<DomainT>::rank::value > 1),
       NViewSubMod<DomainT, SubDim, dash::view_traits<DomainT>::rank::value>
     >::type {
  return NViewSubMod<
           DomainT,
           SubDim,
           dash::view_traits<DomainT>::rank::value
         >(domain,
           begin,
           end);
}

} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
