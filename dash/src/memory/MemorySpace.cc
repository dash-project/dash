#include <dash/memory/HostSpace.h>
#include <dash/memory/HBWSpace.h>

namespace dash {

MemorySpace<dash::memory_space_host_tag>*
get_default_host_space() {
  static HostSpace host_space_singleton;
  return &host_space_singleton;
}

MemorySpace<dash::memory_space_hbw_tag>*
get_default_hbw_space() {
  static HBWSpace hbw_space_singleton;
  return &hbw_space_singleton;
}

} //namespace dash
