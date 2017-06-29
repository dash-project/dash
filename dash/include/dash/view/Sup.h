#ifndef DASH__VIEW__SUP_H__INCLUDED
#define DASH__VIEW__SUP_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainValueT = typename std::decay<DomainT>::type,
  typename SupDomainT   = typename dash::view_traits<DomainValueT>::domain_type,
  typename OriginT      = typename dash::view_traits<DomainValueT>::origin_type
>
constexpr
ViewSubMod<
  SupDomainT,
  SubDim,
  dash::view_traits<DomainValueT>::rank::value >
sup(OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain) {
  return ViewSubMod<
           SupDomainT,
           SubDim,
           dash::view_traits<DomainValueT>::rank::value
         >(dash::domain(
             std::forward<DomainT>(domain)),
           domain.offsets()[SubDim] +
             ( domain.offsets()[SubDim] > 0
               ? begin
               : 0 ),
           domain.offsets()[SubDim] + domain.extents()[SubDim] +
             ( domain.offsets()[SubDim] + domain.extents()[SubDim]
                 < dash::domain(domain).extents()[SubDim]
               ? end
               : 0 ));
}

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetT >
constexpr auto
sup(OffsetT    offset,
    DomainT && domain)
    -> decltype(dash::sup<SubDim>(offset, offset+1, domain)) {
  return dash::sup<SubDim>(offset, offset+1, domain);
}

} // namespace dash

#endif // DASH__VIEW__SUP_H__INCLUDED
