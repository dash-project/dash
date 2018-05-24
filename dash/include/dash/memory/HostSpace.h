#ifndef DASH__MEMORY__HOST_SPACE_H__INCLUDED
#define DASH__MEMORY__HOST_SPACE_H__INCLUDED

#include <dash/memory/MemorySpaceBase.h>

namespace dash {

class HostSpace
  : public dash::MemorySpace<memory_domain_local, memory_space_host_tag> {
public:
  HostSpace()                       = default;
  HostSpace(HostSpace const& other) = default;
  HostSpace(HostSpace&& other)      = default;
  HostSpace& operator=(HostSpace const& other) = default;
  HostSpace& operator=(HostSpace&& other) = default;
  ~HostSpace()                            = default;

protected:
  void* do_allocate(size_t bytes, size_t alignment) override;
  void  do_deallocate(void* p, size_t bytes, size_t alignment) override;
  bool  do_is_equal(cpp17::pmr::memory_resource const& other) const
      noexcept override;
};

}  // namespace dash
#endif  // DASH__MEMORY__HOST_SPACE_H__INCLUDED
