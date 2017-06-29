#ifndef DASH__VIEW__VIEW_CHUNKS_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_CHUNKS_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewMod.h>


namespace dash {

#ifndef DOXYGEN

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------
//
template <
  class DomainType,
  dim_t NDim = dash::view_traits<
                 typename std::decay<DomainType>::type>::rank::value >
class ViewChunksMod;



#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_CHUNKS_MOD_H__INCLUDED
