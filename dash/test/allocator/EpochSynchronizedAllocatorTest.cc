#include "EpochSynchronizedAllocatorTest.h"
#include <dash/memory/HostSpace.h>
#include <dash/memory/HBWSpace.h>
#include <dash/allocator/EpochSynchronizedAllocator.h>
#include <dash/memory/SimpleMemoryPoolResource.h>
#include <cpp17/polymorphic_allocator.h>

TEST_F(EpochSynchronizedAllocatorTest, AllocatorTraits) {
  using Alloc = dash::EpochSynchronizedAllocator<int>;
  using AllocatorTraits = dash::allocator_traits<Alloc>;
  //Rebind from int to double
  using MyAllocTraits = AllocatorTraits::template rebind_traits<double>;
  MyAllocTraits::allocator_type alloc{dash::Team::All()};

  MyAllocTraits::size_type const n_elem = 10;
  MyAllocTraits::local_pointer p =  alloc.allocate_local(n_elem);
  MyAllocTraits::pointer gp = alloc.attach(p, n_elem);
  EXPECT_TRUE_U(!DART_GPTR_ISNULL(gp));

  alloc.detach(gp, n_elem);
  alloc.deallocate_local(p, n_elem);
}


struct my_type {
  double val2;
  int val;
  char c;
};

TEST_F(EpochSynchronizedAllocatorTest, SimplePoolAlloc) {
  using PoolResource = dash::SimpleMemoryPoolResource<dash::HostSpace>;
  using PoolTraits = dash::memory_space_traits<PoolResource>;

  using GlobDynAlloc = dash::EpochSynchronizedAllocator<double, PoolResource>;
  using GlobDynAllocTraits = dash::allocator_traits<GlobDynAlloc>;

  //Global Dynamic Allocator
  PoolResource resource{};
  GlobDynAlloc dynAlloc{dash::Team::All(), &resource};



  GlobDynAllocTraits::size_type const n = 10;
  //Each process allocate 10 local elements
  //Alternative: The Global Allocator allocates 10 elements across the team
  GlobDynAllocTraits::pointer gp = GlobDynAllocTraits::allocate(dynAlloc, n);
  //Each process allocates all the local elements
  GlobDynAllocTraits::deallocate(dynAlloc, gp, n);

  using OtherDynAllocTraits = GlobDynAllocTraits::rebind_traits<my_type>;

  OtherDynAllocTraits::allocator_type otherDynAlloc{dash::Team::All()};
  OtherDynAllocTraits::pointer gp2 = OtherDynAllocTraits::allocate(otherDynAlloc, n);
  OtherDynAllocTraits::deallocate(otherDynAlloc, gp2, n);
}

