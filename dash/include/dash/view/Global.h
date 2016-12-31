#ifndef DASH__VIEW__GLOBAL_H__INCLUDED
#define DASH__VIEW__GLOBAL_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

// Generic definition of \c dash::global:

template <class OriginType>
constexpr OriginType & global(OriginType & origin) {
  return origin;
}

} // namespace dash

#endif // DASH__VIEW__GLOBAL_H__INCLUDED
