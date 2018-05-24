#include <dash/memory/MemorySpace.h>

namespace dash {

template<>
MemorySpace<memory_domain_local, memory_space_host_tag>*
get_default_memory_space<memory_domain_local, memory_space_host_tag>()
{
  static HostSpace host_space_singleton;
  return &host_space_singleton;
}

template<>
MemorySpace<memory_domain_local, memory_space_hbw_tag>*
get_default_memory_space<memory_domain_local, memory_space_hbw_tag>()
{
  static HBWSpace hbw_space_singleton;
  return &hbw_space_singleton;
}

}  // namespace dash
