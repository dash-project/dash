#ifndef DASH__VIEW__APPLY_H__INCLUDED
#define DASH__VIEW__APPLY_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

/**
 * \concept{DashViewConcept}
 */
template <class ViewType>
constexpr typename view_traits<ViewType>::view_type
apply(const ViewType & view) {
  return view.apply();
}

} // namespace dash

#endif // DASH__VIEW__APPLY_H__INCLUDED
