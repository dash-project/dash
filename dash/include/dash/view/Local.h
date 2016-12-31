#ifndef DASH__VIEW__LOCAL_H__INCLUDED
#define DASH__VIEW__LOCAL_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>

#include <dash/view/ViewMod.h>


namespace dash {

template <
  class OriginType >
constexpr typename OriginType::local_type
local(OriginType & origin) {
  return origin.local();
}

} // namespace dash

#endif // DASH__VIEW__LOCAL_H__INCLUDED
