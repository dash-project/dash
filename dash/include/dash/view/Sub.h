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
  class IndexRangeT >
constexpr ViewSubMod<ViewOrigin<NViewDim>, SubDim>
sub(const IndexRangeT & range) {
  return sub<SubDim>(dash::begin(range),
                     dash::end(range));
}
#endif


// -------------------------------------------------------------------------
// View Operations (coupled with origin memory / index space):
// -------------------------------------------------------------------------

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainDecayT = typename std::decay<DomainT>::type >
constexpr
ViewSubMod<
  DomainDecayT,
  SubDim,
  dash::rank<DomainDecayT>::value >
sub(OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain) {
  return ViewSubMod<
           DomainDecayT,
           SubDim,
           dash::view_traits<DomainDecayT>::rank::value
         >(std::forward<DomainT>(domain),
           begin,
           end);
}

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetT,
  typename DomainDecayT = typename std::decay<DomainT>::type,
  typename std::enable_if<
             (!std::is_integral<DomainDecayT>::value &&
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

// -------------------------------------------------------------------------
// Rectangular Region
// -------------------------------------------------------------------------

#if 0
template <
  class DomainT >
auto region(
  DomainT  && domain) {
  return std::forward<DomainT>(domain);
}

/**
 *
 * \code
 * auto reg = matrix | region({ 2,6 }, { 1,5 });
 * // returns sub-matrix within rectangle (2,1), (6,5)
 * \endcode
 */
template <
  class        DomainT,
  typename     IndexT = typename dash::view_traits<DomainT>::index_type,
  dim_t        NDim   = dash::rank<DomainT>::value,
  typename ... Args >
auto region(
  DomainT  && domain,
  const std::array<IndexT,2> & dsub,
  Args ...    subs) {
  return region(
           dash::sub<NDim - sizeof...(Args)>(
             std::get<0>(dsub),
             std::get<1>(dsub),
             std::forward<DomainT>(domain)),
           subs...);
}
#endif

} // namespace dash

#endif // DASH__VIEW__SUB_H__INCLUDED
