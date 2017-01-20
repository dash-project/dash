#ifndef DASH__VIEW__APPLY_H__INCLUDED
#define DASH__VIEW__APPLY_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

/**
 * Inverse operation to \c dash::domain.
 *
 * \concept{DashViewConcept}
 */
template <class ViewTypeA, class ViewTypeB>
constexpr auto apply(
  ViewTypeA & view_a,
  ViewTypeB & view_b) -> decltype(view_a.apply(view_b)) {
  return view_a.apply(view_b);
}

/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr auto apply(
  const ViewType & view) -> decltype(view.apply()) {
  return view.apply();
}

} // namespace dash

#endif // DASH__VIEW__APPLY_H__INCLUDED
