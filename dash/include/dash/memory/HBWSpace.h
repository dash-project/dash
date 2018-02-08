#ifndef DASH__MEMORY__HBW_SPACE_H__INCLUDED
#define DASH__MEMORY__HBW_SPACE_H__INCLUDED

#include <dash/memory/MemorySpace.h>

#ifdef DASH_ENABLE_MEMKIND
#include <hbwmalloc.h>
#else
#include <cstdlib>
#endif

namespace dash {

class HBWSpace : public dash::MemorySpace<void, memory_space_local_domain_tag> {

#ifdef DASH_ENABLE_MEMKIND
  static_assert(alignof(void*) == sizeof(void*), "Required by Memkind");
#endif

public:
  using memory_space_type_category = dash::memory_space_hbw_tag;

public:
  HBWSpace()                      = default;
  HBWSpace(HBWSpace const& other) = default;
  HBWSpace(HBWSpace&& other)      = default;
  HBWSpace& operator=(HBWSpace const& other) = default;
  HBWSpace& operator=(HBWSpace&& other) = default;
  ~HBWSpace()
  {
  }

protected:
  void* do_allocate(size_t bytes, size_t alignment) override;
  void  do_deallocate(void* p, size_t bytes, size_t alignment) override;
  bool  do_is_equal(cpp17::pmr::memory_resource const& other) const
      noexcept override;

private:
  bool check_hbw_available(void);
};

}  // namespace dash
#endif  // DASH__MEMORY__HBW_SPACE_H__INCLUDED
