#ifndef DASH__MEMORY__HOST_SPACE_H__INCLUDED
#define DASH__MEMORY__HOST_SPACE_H__INCLUDED

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/memory/MemorySpace.h>

namespace dash {
namespace memory {

class HostSpace {
 private:
  using block = dash::memory::internal::memory_block;

 public:
  static constexpr unsigned alignment = 4;

 public:
  HostSpace() = default;
  HostSpace(HostSpace const& other) = default;
  HostSpace(HostSpace&& other) = default;
  HostSpace& operator=(HostSpace const& other) = default;
  HostSpace& operator=(HostSpace&& other) = default;

  block allocate(std::size_t const n) noexcept
  {
    block result;

    if (n == 0) {
      return result;
    }
    auto p = std::malloc(n);
    if (p != nullptr) {
      result.ptr = p;
      result.length = n;
      return result;
    }
    return result;
  }

  void deallocate(block& b, std::size_t const nbytes = 0) noexcept
  {
    if (b) {
      std::free(b.ptr);
      b.reset();
    }
  }

  block reallocate(block& b, std::size_t const nbytes) noexcept
  {
    DASH_ASSERT("Not implemented");
    return b;
  }
  /**
   * Allocator Equality: Two HostSpace Allocators are always equal since we use
   * the system heap
   */
  bool operator==(HostSpace const& other) const noexcept { return true; }
  /**
   * Allocator Equality: Two HostSpace Allocators are always equal since we use
   * the system heap
   */
  bool operator!=(HostSpace const& other) const noexcept
  {
    return !(*this == other);
  }
};

}  // memory
}  // dash
#endif  // DASH__MEMORY__HOST_SPACE_H__INCLUDED