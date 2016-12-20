#ifndef DASH__LAUNCH__H__INCLUDED
#define DASH__LAUNCH__H__INCLUDED

namespace dash {

enum class launch : uint16_t {
/// synchronous launch policy
sync     = 0x1,
/// async launch policy
async    = 0x2
};

}


#endif // DASH__LAUNCH__H__INCLUDED