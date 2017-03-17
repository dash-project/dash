#include "DynamicAllocatorTest.h"
#include <dash/memory/MemorySpace.h>
#include <dash/allocator/DynamicAllocator.h>

using Alloc = dash::allocator::DynamicAllocator<int>;
using AllocatorTraits = dash::allocator_traits<Alloc>;

TEST_F(DynamicAllocatorTest, AllocDealloc) {
  //Rebind from int to double
  using MyAllocTraits = AllocatorTraits::template rebind_traits<double>;
  MyAllocTraits::allocator_type alloc{};

  MyAllocTraits::size_type const n_elem = 10;
  MyAllocTraits::local_pointer p =  alloc.allocate_local(n_elem);
  MyAllocTraits::pointer gp = alloc.attach(p, n_elem);
  EXPECT_TRUE_U(gp);

  alloc.detach(gp);
  alloc.deallocate_local(p, n_elem);
}