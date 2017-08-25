#include <dash/memory/HostSpace.h>

namespace dash {

MemorySpace<dash::memory_space_host_tag>*
get_default_host_space()
{
  static HostSpace host_space_singleton;
  return &host_space_singleton;
}
} //namespace dash