#include "DynamicAllocatorTest.h"
#include <dash/memory/MemorySpace.h>
#include <dash/allocator/DynamicAllocator.h>


TEST_F(DynamicAllocatorTest, AllocDealloc) {
  using value_type = int;
  using Alloc = dash::allocator::DynamicAllocator<value_type>;
  Alloc alloc(dash::get_default_memory_space());
}