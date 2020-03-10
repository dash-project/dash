#ifndef DASH__LAUNCH__H__INCLUDED
#define DASH__LAUNCH__H__INCLUDED

#include <cstdint>

namespace dash {

enum class launch
#if !defined(SPEC)
: uint16_t
#endif
{
/// synchronous launch policy
sync     = 0x1,
/// async launch policy
async    = 0x2
};

}


#endif // DASH__LAUNCH__H__INCLUDED
