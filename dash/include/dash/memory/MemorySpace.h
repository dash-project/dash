#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

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

  return dash::GlobPtr<T, memory_t>(*mspace, ptr.dart_gptr());
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

template <class T, class MemorySpaceT>
class DefaultGlobPtrDeleter {
  using size_type = typename MemorySpaceT::size_type;

  MemorySpaceT* m_resource{};
  size_type     m_count{};

public:
  using pointer = typename MemorySpaceT::void_pointer::template rebind<T>;

  constexpr DefaultGlobPtrDeleter() noexcept = default;

  DefaultGlobPtrDeleter(MemorySpaceT* resource, size_type count)
    : m_resource(resource)
    , m_count(count)
  {
    DASH_LOG_TRACE(
        "DefaultGlobPtrDeleter.DefaultGlobPtrDeleter(resource, count)",
        resource,
        count);
  }

  void operator()(pointer gptr)
  {
    if (m_resource && m_count) {
      m_resource->deallocate(gptr, sizeof(T) * m_count, alignof(T));
    }
  }
};

template <class T, class MemorySpaceT>
auto allocate_unique(
    MemorySpaceT* resource, typename MemorySpaceT::size_type count)
    -> std::unique_ptr<T, dash::DefaultGlobPtrDeleter<T, MemorySpaceT>>
{
  // Unique pointer to handle deallocation of underlying resource
  using value_t      = T;
  using memory_t     = MemorySpaceT;
  using deleter_t    = dash::DefaultGlobPtrDeleter<value_t, memory_t>;

  if (resource) {

    auto const nbytes = count * sizeof(value_t);

    return {resource->allocate(nbytes, alignof(value_t)),
            deleter_t{resource, count}};
  }

  return {nullptr};
}

}  // namespace dash

#endif
