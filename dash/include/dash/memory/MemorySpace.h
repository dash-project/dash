#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <dash/memory/HBWSpace.h>
#include <dash/memory/HostSpace.h>

#include <dash/memory/GlobLocalMemoryPool.h>
#include <dash/memory/NeoGlobStaticMemory.h>

#include <dash/GlobPtr.h>

namespace dash {

template <class ElementType, class LMemSpace>
using GlobStaticMem = ::dash::MemorySpace<
    memory_domain_global,
    ElementType,
    allocation_static,
    synchronization_collective,
    LMemSpace>;

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

MemorySpace<
    memory_domain_global,
    uint8_t,
    allocation_static,
    synchronization_independent,
    dash::HostSpace>*
get_default_global_memory_space();

template <typename T>
GlobPtr<
    T,
    MemorySpace<
        memory_domain_global,
        uint8_t,
        allocation_static,
        synchronization_independent,
        dash::HostSpace> >
memalloc(size_t nelem)
{
  auto* mspace   = get_default_global_memory_space();
  using memory_t = std::remove_pointer<decltype(mspace)>::type;
  auto ptr       = mspace->allocate(nelem * sizeof(T));
  return dash::GlobPtr<T, memory_t>(*mspace, ptr.dart_gptr());
}

template <class T>
void memfree(GlobPtr<
             T,
             MemorySpace<
                 memory_domain_global,
                 uint8_t,
                 allocation_static,
                 synchronization_independent,
                 dash::HostSpace> > gptr, size_t nels)
{
  auto * default_mem = get_default_global_memory_space();

  auto freeptr = dash::GlobPtr < uint8_t,
       MemorySpace<
           memory_domain_global,
           uint8_t,
           allocation_static,
           synchronization_independent,
           dash::HostSpace>>{*default_mem, gptr.dart_gptr()};

  get_default_global_memory_space()->deallocate(freeptr, nels * sizeof(T));
}

}  // namespace dash

#endif
