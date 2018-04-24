#ifndef DASH__VIEW__INTERSECT_H__INCLUDED
#define DASH__VIEW__INTERSECT_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/Sub.h>
#include <dash/view/Expand.h>


namespace dash {

/**
 * \concept{DashViewConcept}
 */
template <
  class ViewTypeA,
  class ViewTypeB,
  typename std::enable_if<
             dash::rank<ViewTypeA>::value == 1 &&
             dash::rank<ViewTypeB>::value == 1,
             int >::type = 0
>
constexpr auto
intersect(
  const ViewTypeA & va,
  const ViewTypeB & vb)
  -> decltype(dash::sub(0, 0, va))
{
  return dash::sub(
           dash::index(va).pre()[
             std::max(
               *dash::begin(dash::index(va)),
               *dash::begin(dash::index(vb))
             )
           ],
           dash::index(va).pre()[
             std::min(
               *dash::end(dash::index(va)),
               *dash::end(dash::index(vb))
             )
           ],
           va
         );
}

namespace detail {

template <
  dim_t CurDim,
  class ViewTypeA,
  class ViewTypeB,
  typename std::enable_if< (CurDim < 0), int >::type = 0
>
constexpr auto
intersect_dim(
  ViewTypeA && va,
  ViewTypeB && vb) {
  return std::forward<ViewTypeA>(va);
}

template <
  dim_t CurDim,
  class ViewTypeA,
  class ViewTypeB,
  dim_t NDimA = dash::rank<ViewTypeA>::value,
  dim_t NDimB = dash::rank<ViewTypeB>::value,
  typename std::enable_if<
             (NDimA == NDimB) && (NDimA > 1) && (NDimB > 1) &&
             (CurDim >= 0),
             int >::type = 0
>
constexpr auto
intersect_dim(
  const ViewTypeA  & va,
  ViewTypeB       && vb) {
  return intersect_dim<CurDim - 1>(
           dash::expand<CurDim>(
             std::max<dash::default_index_t>(
               0, std::forward<ViewTypeB>(vb).offsets()[CurDim] -
                  (va).offsets()[CurDim]
             ),
             std::min<dash::default_index_t>(
               0, ( std::forward<ViewTypeB>(vb).offsets()[CurDim] +
                    std::forward<ViewTypeB>(vb).extents()[CurDim] ) -
                  ( (va).offsets()[CurDim] +
                    (va).extents()[CurDim] )
             ),
             (va)
           ),
           std::forward<ViewTypeB>(vb));
}

} // namespace detail

template <
  class ViewTypeA,
  class ViewTypeB,
  dim_t NDimA = dash::rank<ViewTypeA>::value,
  dim_t NDimB = dash::rank<ViewTypeB>::value,
  typename std::enable_if<
             (NDimA == NDimB) && (NDimA > 1) && (NDimB > 1),
             int >::type = 0
>
constexpr auto
intersect(
  const ViewTypeA  & va,
  ViewTypeB       && vb) {
  return detail::intersect_dim<NDimA - 1>(
           va,
           std::forward<ViewTypeB>(vb));
}

template <
  class ViewType >
static inline auto intersect(const ViewType & v_rhs) {
  return dash::make_pipeable(
           [=](auto && v_lhs) {
             return intersect(
                      std::forward<decltype(v_lhs)>(v_lhs),
                      v_rhs);
           });
}

} // namespace dash

#endif // DASH__VIEW__INTERSECT_H__INCLUDED
