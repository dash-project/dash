#ifndef DASH__VIEW__EXPAND_H__INCLUDED
#define DASH__VIEW__EXPAND_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>
#include <dash/view/Utility.h>
#include <dash/view/Sub.h>


namespace dash {

/**
 * Origin cannot be expanded.
 *
 */
template <
  dim_t    SubDim  = 0,
  class    OriginT,
  class    OffsetFirstT,
  class    OffsetFinalT,
  typename OriginValueT
            = typename std::decay<OriginT>::type
>
constexpr auto
expand(
  OffsetFirstT    begin,
  OffsetFinalT    end,
  OriginT      && origin)
  -> typename std::enable_if<
                dash::view_traits<OriginValueT>::is_origin::value,
                decltype(std::forward<OriginT>(origin))
              >::type {
  return std::forward<OriginT>(origin);
}


namespace detail {
  
  template <
    dim_t    CurDim,
    dim_t    SubDim,
    class    DomainT,
    class    OffsetFirstT,
    class    OffsetFinalT,
    typename DomainValueT
              = typename std::decay<DomainT>::type,
    typename IndexT
              = typename DomainValueT::index_type,
    typename SizeT
              = typename DomainValueT::size_type,
    typename OffsetsT,
    typename ExtentsT,
    typename std::enable_if< CurDim == 0, char >::type = 0
  >
  constexpr auto
  expand_dim(
    OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain,
    OffsetsT        cur_offsets,
    ExtentsT        cur_extents) {
    return ( CurDim == SubDim
             // span in current dimension is to be expanded
             ? dash::sub<CurDim>(
                 ( cur_offsets[CurDim] + begin > 0
                   ? cur_offsets[CurDim] + begin
                   : 0 ),
                 ( cur_offsets[CurDim] + cur_extents[CurDim] + end <
                     domain.extents()[CurDim]
                   ? cur_offsets[CurDim] + cur_extents[CurDim] + end
                   : domain.extents()[CurDim] ),
                 std::forward<DomainT>(domain))
             // span in current dimension is unchanged
             : dash::sub<CurDim>(
                 (cur_offsets[CurDim]),
                 (cur_offsets[CurDim] + cur_extents[CurDim]),
                 std::forward<DomainT>(domain))
           );
  }

  template <
    dim_t    CurDim,
    dim_t    SubDim,
    class    DomainT,
    class    OffsetFirstT,
    class    OffsetFinalT,
    typename DomainValueT
              = typename std::decay<DomainT>::type,
    typename IndexT
              = typename DomainValueT::index_type,
    typename SizeT
              = typename DomainValueT::size_type,
    typename OffsetsT,
    typename ExtentsT,
    typename std::enable_if< (CurDim > 0), char >::type = 0
  >
  constexpr auto
  expand_dim(
    OffsetFirstT    begin,
    OffsetFinalT    end,
    DomainT      && domain,
    OffsetsT        cur_offsets,
    ExtentsT        cur_extents) {
    return expand_dim<CurDim - 1, SubDim>(
             static_cast<IndexT>(begin),
             static_cast<IndexT>(end),
             ( CurDim == SubDim
               // span in current dimension is to be expanded
               ? dash::sub<CurDim>(
                   ( cur_offsets[CurDim] + begin > 0
                     ? cur_offsets[CurDim] + begin
                     : 0 ),
                   ( cur_offsets[CurDim] + cur_extents[CurDim] + end <
                       domain.extents()[CurDim]
                     ? cur_offsets[CurDim] + cur_extents[CurDim] + end
                     : domain.extents()[CurDim] ),
                   std::forward<DomainT>(domain))
               // span in current dimension is unchanged
               : dash::sub<CurDim>(
                   (cur_offsets[CurDim]),
                   (cur_offsets[CurDim] + cur_extents[CurDim]),
                   std::forward<DomainT>(domain))
             ),
             cur_offsets,
             cur_extents);
  }

} // namespace detail


template <
  dim_t    SubDim  = 0,
  class    DomainT,
//class    OffsetFirstT,
//class    OffsetFinalT,
  typename DomainValueT
            = typename std::decay<DomainT>::type,
  typename IndexT
            = typename DomainValueT::index_type,
  typename SizeT
            = typename DomainValueT::size_type,
  dim_t    NDim
            = dash::view_traits<DomainValueT>::rank::value,
  typename std::enable_if<
             !dash::view_traits<DomainValueT>::is_origin::value,
             char >::type = 0
>
constexpr auto
expand(
  IndexT          begin,
  SizeT           end,
  DomainT      && domain) {
  return detail::expand_dim<
           static_cast<dim_t>(NDim - 1),
           static_cast<dim_t>(SubDim)
         >(begin, end,
           dash::origin(
             std::forward<DomainT>(domain)),
          (domain.offsets()),
          (domain.extents()));
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
           dash::view_traits<SecDomainT>::rank::value
         >(dash::domain(
             std::forward<DomainT>(domain)),
           domain.offsets()[SubDim] +
             ( domain.offsets()[SubDim] + begin > 0
               ? begin
               : 0 ),
           domain.offsets()[SubDim] + domain.extents()[SubDim] +
             ( domain.offsets()[SubDim] + domain.extents()[SubDim] + end
                 < dash::domain(domain).extents()[SubDim]
               ? end
               : 0 ));
}





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

template <
  dim_t    SubDim  = 0,
  class    OffsetT0,
  class    OffsetT1,
  typename std::enable_if<
             ( std::is_integral<OffsetT0>::value &&
               std::is_integral<OffsetT1>::value ),
             int
           >::type = 0 >
static inline auto expand(OffsetT0 a, OffsetT1 b) {
  return dash::make_pipeable(
           [=](auto && x) {
             return expand<SubDim>(a,b, std::forward<decltype(x)>(x));
           });
}

} // namespace dash

#endif // DASH__VIEW__EXPAND_H__INCLUDED
