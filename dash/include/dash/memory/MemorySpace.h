#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <dash/memory/HBWSpace.h>
#include <dash/memory/HostSpace.h>

#include <dash/memory/NeoGlobStaticMemory.h>

namespace dash {
namespace experimental {

template <class ElementType, class LMemSpace>
using GlobStaticMem = ::dash::MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    synchronization_collective,
    LMemSpace>;
}

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
//

// TODO rko: maybe there is a better way instead of all these specializations?

namespace detail {
template <class, class>
struct dependent_false : std::false_type {
};
}  // namespace detail

template <class MSpaceDomainCategory, class MSpaceTypeCategory>
inline MemorySpace<MSpaceDomainCategory, MSpaceTypeCategory>*
get_default_memory_space()
{
  static_assert(
      detail::dependent_false<MSpaceDomainCategory, MSpaceTypeCategory>::
          value,
      "No default memory space for this configuration available");
  // Current we have only a default host space
  return nullptr;
}

template <>
MemorySpace<memory_domain_local, memory_space_host_tag>*
get_default_memory_space<memory_domain_local, memory_space_host_tag>();

template <>
MemorySpace<memory_domain_local, memory_space_hbw_tag>*
get_default_memory_space<memory_domain_local, memory_space_hbw_tag>();
}  // namespace dash

#endif
