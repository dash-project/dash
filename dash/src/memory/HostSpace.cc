#include <dash/memory/HostSpace.h>

void * dash::HostSpace::do_allocate(size_t bytes, size_t alignment)
{
  return std::pmr::get_default_resource()->allocate(bytes, alignment);
}
void dash::HostSpace::do_deallocate(void* p, size_t bytes, size_t alignment)
{
  std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
}
bool dash::HostSpace::do_is_equal(std::pmr::memory_resource const& other) const
    noexcept
{
  return std::pmr::get_default_resource()->is_equal(other);
}
