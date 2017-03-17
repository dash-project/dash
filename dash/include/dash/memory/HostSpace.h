#ifndef DASH__MEMORY__HOST_SPACE_H__INCLUDED
#define DASH__MEMORY__HOST_SPACE_H__INCLUDED

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/memory/MemorySpace.h>
#include <stdlib.h>

namespace dash {

class HostSpace : public dash::MemorySpace<dash::memory_space_host_tag>{
 private:
  using void_pointer = HostSpace::void_pointer;
  using Base = dash::MemorySpace<dash::memory_space_host_tag>;

 public:
  HostSpace() = default;
  HostSpace(HostSpace const& other) = default;
  HostSpace(HostSpace&& other) = default;
  HostSpace& operator=(HostSpace const& other) = default;
  HostSpace& operator=(HostSpace&& other) = default;
  ~HostSpace() {}

  void_pointer allocate(std::size_t const n, std::size_t alignment = alignof(std::max_align_t)) noexcept
  {
    if (n == 0) {
      return nullptr;
    }
    auto p = aligned_alloc(alignment, n);
    return p;
  }

  void deallocate(void_pointer p, std::size_t bytes) noexcept
  {
    if (p) {
      std::free(p);
    }
  }

  /**
   * Space Equality: Two HostSpaces are always equal since we use
   * the system heap
   */
  bool is_equal(Base const& other) const noexcept { return true; }
};

}  // dash
#endif  // DASH__MEMORY__HOST_SPACE_H__INCLUDED