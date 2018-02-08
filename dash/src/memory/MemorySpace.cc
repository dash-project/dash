#include <dash/memory/HostSpace.h>
#include <dash/memory/HBWSpace.h>

namespace dash {

MemorySpace<void, memory_space_local_domain_tag>*
get_default_host_space() {
  static HostSpace host_space_singleton;
  return &host_space_singleton;
}

MemorySpace<void, memory_space_local_domain_tag>*
get_default_hbw_space() {
  static HBWSpace hbw_space_singleton;
  return &hbw_space_singleton;
}

} //namespace dash
