#ifndef DASH__ALLOCATOR__BASE_H__INCLUDED
#define DASH__ALLOCATOR__BASE_H__INCLUDED

#include <dash/Types.h>
#include <dash/allocator/AllocationPolicy.h>
#include <dash/memory/MemorySpace.h>

namespace dash {

namespace allocator {

template <typename T>
using polymorphic_allocator = cpp17::pmr::polymorphic_allocator<T>;

template <typename T>
class DefaultAllocator : public polymorphic_allocator<T> {
  using base_t = polymorphic_allocator<T>;

public:
  DefaultAllocator() = default;

  template <class MemSpaceT>
  explicit DefaultAllocator(MemSpaceT* r)
    : base_t(r)
  {
  }
};

}  // namespace allocator

}  // namespace dash
#endif
