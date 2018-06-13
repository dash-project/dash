#ifndef DASH__VIEW__SET_DIFFERENCE_H__INCLUDED
#define DASH__VIEW__SET_DIFFERENCE_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/Sub.h>


namespace dash {

/**
 * \concept{DashViewConcept}
 */
template <
  class ViewTypeA,
  class ViewTypeB >
constexpr auto
difference(
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

} // namespace dash

#endif // DASH__VIEW__SET_DIFFERENCE_H__INCLUDED
