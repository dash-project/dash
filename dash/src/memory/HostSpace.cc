#include <dash/memory/HostSpace.h>

void * dash::HostSpace::do_allocate(size_t bytes, size_t alignment)
{
  return cpp17::pmr::get_default_resource()->allocate(bytes, alignment);
}
void dash::HostSpace::do_deallocate(void* p, size_t bytes, size_t alignment)
{
  cpp17::pmr::get_default_resource()->deallocate(p, bytes, alignment);
}
bool dash::HostSpace::do_is_equal(cpp17::pmr::memory_resource const& other) const
    noexcept
{
  return cpp17::pmr::get_default_resource()->is_equal(other);
}
