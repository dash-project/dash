#ifndef DASH__MEMORY__HOST_SPACE_H__INCLUDED
#define DASH__MEMORY__HOST_SPACE_H__INCLUDED

#include <cstdlib>

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/memory/MemorySpace.h>
#include <dash/memory/internal/Util.h>

namespace dash {

class HostSpace
  : public dash::MemorySpace<
             dash::memory_space_host_tag >
{
 private:
  using void_pointer = HostSpace::void_pointer;
  using base_t       = dash::MemorySpace<
                         dash::memory_space_host_tag >;

 public:

 public:
  HostSpace() = default;
  HostSpace(HostSpace const& other) = default;
  HostSpace(HostSpace&& other) = default;
  HostSpace& operator=(HostSpace const& other) = default;
  HostSpace& operator=(HostSpace&& other) = default;
  ~HostSpace() {}

  void_pointer do_allocate(
    std::size_t const n,
    std::size_t alignment = alignof(dash::max_align_t))
  {
    DASH_LOG_DEBUG("HostSpace.do_allocate(n, alignment)", n, alignment);
    if (n == 0) {
      return nullptr;
    }

    void_pointer ptr;
    if (alignment < alignof(void *)) {
      alignment = alignof(void *);
    } else {
      alignment = dash::memory::internal::next_power_of_2(alignment);
    }

    ptr = aligned_alloc(alignment, n);
    if (nullptr == ptr) {
      DASH_LOG_ERROR("HostSpace.do_allocate(n, alignment) --> Cannot allocate memory", n, alignment);
      throw std::bad_alloc();
    }
    DASH_LOG_TRACE("HostSpace.do_allocate(n, alignment)", "Allocated memory segment(pointer, nbytes, alignment)", ptr, n, alignment);
    DASH_LOG_DEBUG("HostSpace.do_allocate(n, alignment) >");
    return ptr;
  }

  void do_deallocate(void_pointer p, std::size_t, std::size_t alignment)
  {
    DASH_LOG_DEBUG("HostSpace.do_deallocate(p, size_t, alignment)", p, alignment);
    if (p) {
      std::free(p);
    }
    DASH_LOG_DEBUG("HostSpace.do_deallocate(p, size_t, alignment) >", p, alignment);
  }

  /**
   * Space Equality: Two HostSpaces are always equal since we use
   * the system heap
   */
  bool is_equal(base_t const & other) const noexcept { return true; }

  size_type size() const { return 0;};
  size_type size(dash::team_unit_t unit) const { return 0;};
};

}  // dash
#endif  // DASH__MEMORY__HOST_SPACE_H__INCLUDED
