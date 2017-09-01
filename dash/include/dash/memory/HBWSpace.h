#ifndef DASH__MEMORY__HBW_SPACE_H__INCLUDED
#define DASH__MEMORY__HBW_SPACE_H__INCLUDED

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/memory/MemorySpace.h>

#ifdef DASH_ENABLE_MEMKIND
#include <hbwmalloc.h>
#else
#include <cstdlib>
#endif


namespace dash {

class HBWSpace
  : public dash::MemorySpace<
             dash::memory_space_hbw_tag >
{
 private:
  using void_pointer = HBWSpace::void_pointer;
  using base_t       = dash::MemorySpace<
                         dash::memory_space_hbw_tag >;

 public:

 public:
  HBWSpace() = default;
  HBWSpace(HBWSpace const& other) = default;
  HBWSpace(HBWSpace&& other) = default;
  HBWSpace& operator=(HBWSpace const& other) = default;
  HBWSpace& operator=(HBWSpace&& other) = default;
  ~HBWSpace() {}

  void_pointer do_allocate(
    std::size_t const n,
    std::size_t alignment = alignof(dash::max_align_t))
  {
    DASH_LOG_DEBUG("HBWSpace.do_allocate(n, alignment)", n, alignment);

    if (n == 0) {
      return nullptr;
    }

    void_pointer ptr;

    if (alignment < alignof(void *)) {
      alignment = alignof(void *);
    } else {
      alignment = dash::memory::internal::next_power_of_2(alignment);
    }

#ifdef DASH_ENABLE_MEMKIND
    auto ret = hbw_posix_memalign(&ptr, alignment, n);
#else
    DASH_LOG_WARN("HBWSpace.do_allocate(n, alignment)",
        "hbw_malloc is not available --> fall back to std::malloc");

    auto ret = posix_memalign(&ptr, alignment, n);
#endif
    if (ret) {
      DASH_LOG_ERROR("HBWSpace.do_allocate(n, alignment) --> Cannot allocate memory", n, alignment);
      throw std::bad_alloc();
    }
    DASH_LOG_TRACE("HBWSpace.do_allocate(n, alignment)", "Allocated memory segment(pointer, nbytes, alignment)", ptr, n, alignment);
    DASH_LOG_DEBUG("HBWSpace.do_allocate(n, alignment) >");
    return ptr;
  }

  void do_deallocate(void_pointer p, std::size_t, std::size_t alignment)
  {
    if (p) {
#ifdef DASH_ENABLE_MEMKIND
      hbw_free(p);
#else
      std::free(p);
#endif
    }
  }

  /**
   * Space Equality: Two HBWSpace are always equal since we use
   * the system heap
   */
  bool is_equal(base_t const & other) const noexcept { return true; }

  size_type size() const { return 0;};
};

}  // dash
#endif  // DASH__MEMORY__HBW_SPACE_H__INCLUDED
