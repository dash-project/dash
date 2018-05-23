#ifndef DASH__ALLOCATOR__BASE_H__INCLUDED
#define DASH__ALLOCATOR__BASE_H__INCLUDED

#include <dash/Types.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpace.h>

namespace dash {

namespace allocator {

template <typename T>
using polymorphic_allocator = cpp17::pmr::polymorphic_allocator<T>;

template <typename T, typename MemSpaceT>
class DefaultAllocator : public polymorphic_allocator<T> {
  using memory_traits = typename dash::memory_space_traits<MemSpaceT>;

  static_assert(memory_traits::is_local::value, "memory space must be local");

  using base_t = polymorphic_allocator<T>;

public:
  DefaultAllocator() = default;
  explicit DefaultAllocator(MemSpaceT* r)
    : base_t(r)
  {
  }

public:
  using memory_space = MemSpaceT;
};

}  // namespace allocator

}  // namespace dash
#endif
