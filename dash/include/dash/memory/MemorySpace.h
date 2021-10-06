#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <dash/memory/CudaSpace.h>
#include <dash/memory/HBWSpace.h>
#include <dash/memory/HostSpace.h>

#include <dash/memory/GlobLocalMemoryPool.h>
#include <dash/memory/GlobStaticMem.h>

#include <dash/GlobPtr.h>

namespace dash {

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

template <>
MemorySpace<memory_domain_local, memory_space_cuda_tag>*
get_default_memory_space<memory_domain_local, memory_space_cuda_tag>();

template <>
MemorySpace<memory_domain_global, memory_space_host_tag>*
get_default_memory_space<memory_domain_global, memory_space_host_tag>();

template <typename T>
GlobPtr<T, GlobLocalMemoryPool<dash::HostSpace>> memalloc(size_t nelem)
{
  using memory_t = GlobLocalMemoryPool<dash::HostSpace>;

  auto* mspace = dynamic_cast<memory_t*>(get_default_memory_space<
                                         memory_domain_global,
                                         memory_space_host_tag>());

  DASH_ASSERT_MSG(mspace, "invalid default memory space");

  auto ptr = mspace->allocate(nelem * sizeof(T), alignof(T));

  return static_cast<dash::GlobPtr<T, memory_t>>(ptr);
}

template <class T>
void memfree(
    GlobPtr<T, GlobLocalMemoryPool<dash::HostSpace>> gptr, size_t nels)
{
  using memory_t = GlobLocalMemoryPool<dash::HostSpace>;

  auto* mspace = dynamic_cast<memory_t*>(get_default_memory_space<
                                         memory_domain_global,
                                         memory_space_host_tag>());

  DASH_ASSERT_MSG(mspace, "invalid default memory space");

  mspace->deallocate(gptr, nels * sizeof(T), alignof(T));
}

/**
 * Convenience Wrapper to retrieve easily the type allocated by
 * dash::memalloc<T>
 *
 */
template <class T>
using GlobMemAllocPtr = decltype(dash::memalloc<T>(size_t{}));

}  // namespace dash

#endif
