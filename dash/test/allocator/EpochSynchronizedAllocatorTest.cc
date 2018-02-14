#include "EpochSynchronizedAllocatorTest.h"
#include <dash/memory/HostSpace.h>
#include <dash/memory/HBWSpace.h>
#include <dash/allocator/EpochSynchronizedAllocator.h>
#include <dash/memory/SimpleMemoryPool.h>
#include <cpp17/polymorphic_allocator.h>

#if 0

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

//C++17 polymorphic_allocator
template <typename T>
using PMA = cpp17::pmr::polymorphic_allocator<T>;

// C++17 string that uses a polymorphic allocator
template <class charT, class traits = std::char_traits<charT>>
using basic_string = std::basic_string<charT, traits,
                                       PMA<charT>>;
using string  = basic_string<char>;
using wstring = basic_string<wchar_t>;


TEST_F(EpochSynchronizedAllocatorTest, PolyAlloc) {

  dash::HBWSpace hbwSpace{};
  std::vector<size_t, PMA<size_t>> vec {10, 0, &hbwSpace};

  std::iota(std::begin(vec), std::end(vec), 0);

  for (std::size_t idx = 0; idx < 10; ++idx) {
    ASSERT_EQ_U(vec[idx], idx);
  }

  std::vector<string, PMA<string>> strvec{&hbwSpace};

  for (std::size_t idx = 0; idx < 10; ++idx) {
    std::stringstream ss;
    ss << "Element: " << idx;

    strvec.push_back(string{ss.str().c_str()});
  }
  for (std::size_t idx = 0; idx < 10; ++idx) {
    DASH_LOG_DEBUG("Value", idx, strvec[idx]);
  }

}
#endif

