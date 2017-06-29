#ifndef DASH__VIEW__EXPAND_H__INCLUDED
#define DASH__VIEW__EXPAND_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainValueT
            = typename std::decay<DomainT>::type,
  typename SecDomainT
            = typename dash::view_traits<DomainValueT>::domain_type
>
constexpr auto
expand(
  OffsetFirstT    begin,
  OffsetFinalT    end,
  DomainT      && domain)
  -> typename std::enable_if<
                true ||
                dash::view_traits<
                  typename dash::view_traits<DomainValueT>::domain_type
                >::is_origin::value,
                ViewSubMod<SecDomainT, SubDim,
                           dash::view_traits<SecDomainT>::rank::value >
              >::type {
  return ViewSubMod<
           SecDomainT,
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

#if 0
template <
  dim_t    SubDim  = 0,
  class    DomainT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename DomainValueT
            = typename std::decay<DomainT>::type,
  typename SecDomainT
            = typename dash::view_traits<DomainValueT>::domain_type
>
constexpr auto
expand(
  OffsetFirstT    begin,
  OffsetFinalT    end,
  DomainT      && domain)
  -> typename std::enable_if<
                !dash::view_traits<
                   typename dash::view_traits<DomainValueT>::domain_type
                 >::is_origin::value,
                ViewSubMod<DomainValueT, SubDim,
                           dash::view_traits<DomainValueT>::rank::value >
              >::type {
  return ViewSubMod<
           DomainValueT,
           SubDim,
           dash::view_traits<DomainValueT>::rank::value
         >(std::forward<DomainT>(domain),
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
#endif

} // namespace dash

#endif // DASH__VIEW__EXPAND_H__INCLUDED
