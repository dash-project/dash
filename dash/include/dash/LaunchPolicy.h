#ifndef DASH__LAUNCH_POLICY_H__INCLUDED
#define DASH__LAUNCH_POLICY_H__INCLUDED

#include <cstdint>

namespace dash {

enum class launch : uint16_t {
  /// synchronous launch policy
  sync      = 0x1,
  /// async launch policy
  async     = 0x2,
  /// deferred launch policy
  deferred  = 0x4,
  /// unspecified launch policy
  any       = sync | async | deferred
};

}


#endif // DASH__LAUNCH_POLICY_H__INCLUDED
