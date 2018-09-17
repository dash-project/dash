#include <dash/memory/MemorySpace.h>

namespace dash {

template <>
MemorySpace<memory_domain_local, memory_space_host_tag>*
get_default_memory_space<memory_domain_local, memory_space_host_tag>()
{
  static HostSpace host_space_singleton;
  return &host_space_singleton;
}

template <>
MemorySpace<memory_domain_local, memory_space_hbw_tag>*
get_default_memory_space<memory_domain_local, memory_space_hbw_tag>()
{
  static HBWSpace hbw_space_singleton;
  return &hbw_space_singleton;
}

template <>
MemorySpace<memory_domain_global, memory_space_host_tag> *
get_default_memory_space<memory_domain_global, memory_space_host_tag>()
{
  using memory_t = dash::GlobLocalMemoryPool<uint8_t, dash::HostSpace>;

  static memory_t globmem{0, dash::Team::All()};

  return &globmem;
}

}  // namespace dash
