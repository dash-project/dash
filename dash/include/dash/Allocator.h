#ifndef DASH__ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR_H__INCLUDED

#include <dash/allocator/AllocatorTraits.h>

#include <dash/allocator/SymmetricAllocator.h>
#include <dash/allocator/EpochSynchronizedAllocator.h>

#include <dash/memory/MemorySpace.h>

#include <dash/memory/SimpleMemoryPoolResource.h>

namespace dash {

/**
 * Replacement for missing std::align in gcc < 5.0.
 *
 * Implementation from original bug report:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57350
 */
inline void * align(
  std::size_t   alignment,
  std::size_t   size,
  void *      & ptr,
  std::size_t & space)
{
  auto pn = reinterpret_cast< std::uintptr_t >(ptr);
  auto aligned   = (pn + alignment - 1) & - alignment;
  auto new_space = space - ( aligned - pn );
  if (new_space < size) return nullptr;
  space      = new_space;
  return ptr = reinterpret_cast<void *>(aligned);
}

template <
    typename T,
    typename MS                              = dash::HostSpace,
    template <class, class> class LocalAlloc = allocator::DefaultAllocator>
using LocalAllocator = dash::SymmetricAllocator<
    T,
    dash::global_allocation_policy::non_collective,
    MS,
    LocalAlloc>;

} // namespace dash

#endif // DASH__ALLOCATOR_H__INCLUDED
