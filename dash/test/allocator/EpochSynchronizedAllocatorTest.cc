#include "EpochSynchronizedAllocatorTest.h"
#include <dash/memory/HostSpace.h>
#include <dash/allocator/EpochSynchronizedAllocator.h>
#include <dash/memory/SimpleMemoryPool.h>


TEST_F(EpochSynchronizedAllocatorTest, AllocDealloc) {
  using Alloc = dash::allocator::EpochSynchronizedAllocator<int>;
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
  using LocalAlloc = dash::allocator::LocalSpaceAllocator<double, dash::memory_space_host_tag>;
  using PoolAlloc = dash::SimpleMemoryPool<double, LocalAlloc>;
  using GlobDynAlloc = dash::allocator::EpochSynchronizedAllocator<double, dash::memory_space_host_tag, PoolAlloc>;
  using GlobDynAllocTraits = dash::allocator_traits<GlobDynAlloc>;

  //Memory Space
  dash::HostSpace hostSpace{};
  //local allocator
  LocalAlloc localAlloc{&hostSpace};
  // pool allocator
  PoolAlloc pool{localAlloc};
  //Global Dynamic Allocator
  GlobDynAlloc dynAlloc{pool};


  GlobDynAllocTraits::size_type const n = 10;
  //Each process allocate 10 local elements
  //Alternative: The Global Allocator allocates 10 elements across the team
  GlobDynAllocTraits::pointer gp = GlobDynAllocTraits::allocate(dynAlloc, n);
  //Each process allocates all the local elements
  GlobDynAllocTraits::deallocate(dynAlloc, gp, n);

  using OtherDynAllocTraits = GlobDynAllocTraits::rebind_traits<struct my_type>;

  OtherDynAllocTraits::allocator_type otherDynAlloc{dash::Team::All()};
  OtherDynAllocTraits::pointer gp2 = OtherDynAllocTraits::allocate(otherDynAlloc, n);
  OtherDynAllocTraits::deallocate(otherDynAlloc, gp2, n);
}

