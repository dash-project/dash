#include "GlobStaticMemTest.h"
#include <dash/GlobRef.h>
#include <dash/allocator/GlobalAllocator.h>
#include <dash/memory/UniquePtr.h>

TEST_F(GlobStaticMemTest, GlobalRandomAccess)
{
  auto globmem_local_elements = {1, 2, 3};
  using value_t = typename decltype(globmem_local_elements)::value_type;

  using memory_t    = dash::GlobStaticMem<dash::HostSpace>;
  using allocator_t = dash::GlobalAllocator<value_t, memory_t>;
  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;
  // local pointer type
  using lptr_t = typename std::pointer_traits<
      typename memory_t::local_void_pointer>::template rebind<value_t>;

  auto const nlelem = globmem_local_elements.size();
  auto const ngelem = dash::size() * nlelem;

  memory_t    globmem{dash::Team::All()};
  allocator_t alloc{&globmem};

  auto const ptr_alloc = alloc.allocate(globmem_local_elements.size());

  EXPECT_TRUE_U(ptr_alloc);

  auto soon_to_be_lbegin = ptr_alloc;
  soon_to_be_lbegin.set_unit(dash::Team::All().myid());
  auto *lbegin = soon_to_be_lbegin.local();

  EXPECT_TRUE_U(lbegin);

  std::uninitialized_copy(
      std::begin(globmem_local_elements),
      std::end(globmem_local_elements),
      lbegin);

  globmem.barrier();

  DASH_LOG_DEBUG_VAR("GlobStaticMemTest", globmem.capacity());
  EXPECT_EQ_U(globmem.capacity(), 3 * dash::size() * sizeof(value_t));

  if (dash::myid() == 0) {
    auto gbegin = ptr_alloc;
    auto glast  = gbegin + (ngelem - 1);
    auto gend   = gbegin + ngelem;

    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gbegin);
    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", glast);
    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gend);

    // Test distance in global memory over multiple unit spaces:
    EXPECT_EQ(gend - gbegin, ngelem);
    EXPECT_EQ(glast - gbegin, ngelem - 1);

    // Iterate entire global memory space, including end pointer:
    for (int g = 0; g <= ngelem; ++g) {
      DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gbegin);
      // Do not dereference end pointer:
      if (g < ngelem) {
        int gvalue = *gbegin;
        EXPECT_EQ((g % 3) + 1, gvalue);
        EXPECT_EQ(*gbegin, ptr_alloc[g]);
      }
      EXPECT_EQ(gbegin, ptr_alloc + g);

      EXPECT_EQ(
          (static_cast<dash::gptrdiff_t>(ngelem) - g),
          dash::distance(gbegin, gend));
      EXPECT_EQ(
          -(static_cast<dash::gptrdiff_t>(ngelem) - g),
          dash::distance(gend, gbegin));
      EXPECT_EQ(gend - gbegin, dash::distance(gbegin, gend));
      EXPECT_EQ(gbegin - gend, dash::distance(gend, gbegin));

      if (g % 2 == 0) {
        ++gbegin;
      }
      else {
        gbegin++;
      }
    }
  }

  globmem.barrier();

  if (dash::myid() == dash::size() - 1) {
    auto gbegin = ptr_alloc;
    auto gend   = gbegin + ngelem;
    // Reverse iteratation on entire global memory space, starting at
    // end pointer:
    for (int g = ngelem; g >= 0; --g) {
      DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gend);
      // Do not dereference end pointer:
      if (g < ngelem) {
        value_t gvalue = *gend;
        EXPECT_EQ((g % 3) + 1, gvalue);
      }
      EXPECT_EQ(gend, ptr_alloc + g);

      EXPECT_EQ(gend - gbegin, dash::distance(gbegin, gend));
      EXPECT_EQ(gbegin - gend, dash::distance(gend, gbegin));

      if (g % 2 == 0) {
        --gend;
      }
      else {
        gend--;
      }
    }
  }

  alloc.deallocate(ptr_alloc, globmem_local_elements.size());
}

TEST_F(GlobStaticMemTest, LocalBegin)
{
  auto target_local_elements = {1, 2, 3, 4};

  using value_t     = typename decltype(target_local_elements)::value_type;
  using memory_t    = dash::GlobStaticMem<dash::HostSpace>;
  using allocator_t = dash::GlobalAllocator<value_t, memory_t>;

  // global pointer type
  if (!dash::Team::All().is_leaf()) {
    SKIP_TEST_MSG("Team is already split");
  }

  auto &sub_team =
      dash::size() < 4 ? dash::Team::All() : dash::Team::All().split(2);

  memory_t    target{dash::Team::All()};
  allocator_t alloc{&target};

  auto const gptr = alloc.allocate(target_local_elements.size());

  EXPECT_TRUE_U(gptr);

  auto soon_to_be_lbegin = gptr;
  soon_to_be_lbegin.set_unit(target.team().myid());

  auto *lbegin = soon_to_be_lbegin.local();

  EXPECT_NE_U(lbegin, nullptr);

  std::uninitialized_copy(
      std::begin(target_local_elements),
      std::end(target_local_elements),
      lbegin);

  target.barrier();

  for (int l = 0; l < target_local_elements.size(); l++) {
    EXPECT_EQ_U(*(gptr + l), lbegin[l]);
  }

  alloc.deallocate(gptr, target_local_elements.size());
}

TEST_F(GlobStaticMemTest, MakeUniqueMoveSemantics)
{
  // We always use an allocator of byte..
  using allocator =
      dash::GlobalAllocator<std::byte, dash::GlobStaticMem<dash::HostSpace>>;
  using memory_resource = typename allocator::memory_resource;

  using value_t = int;

  memory_resource resourceA{dash::Team::All()};
  memory_resource resourceB{dash::Team::All()};

  allocator allocA{&resourceA};
  allocator allocB{&resourceB};

  constexpr size_t lcapA = 20;
  constexpr size_t lcapB = lcapA / 2;

  auto const cap_in_bytes = [](size_t lcap) {
    return lcap * sizeof(value_t);
  };

  // Allocate first pointer
  LOG_MESSAGE("Allocating memory segment 1");
  auto p1 = dash::allocate_unique<value_t>(allocA, lcapA);
  EXPECT_TRUE_U(p1);

  auto *lp1 = dash::local_begin(p1.get(), dash::Team::All().myid());
  EXPECT_TRUE_U(lp1);

  // allocated capacity
  EXPECT_EQ_U(resourceA.capacity(), cap_in_bytes(lcapA) * dash::size());
  EXPECT_EQ_U(
      resourceA.capacity(dash::Team::All().myid()), cap_in_bytes(lcapA));

  // validate memory space registry
  auto const &reg = dash::internal::MemorySpaceRegistry::GetInstance();
  EXPECT_EQ_U(reg.lookup(static_cast<dart_gptr_t>(p1.get())), &resourceA);

  constexpr value_t value = 123;

  // Initialize some in segment 1
  std::uninitialized_fill(lp1, std::next(lp1, lcapA), value);

  // Prepare values for segment 2
  std::vector<value_t> values(lcapB);
  std::iota(std::begin(values), std::end(values), dash::myid() * lcapB);

  /*
   * Swap two Unique Pointers
   */
  {
    LOG_MESSAGE("Allocating memory segment 2");
    auto p2 = dash::allocate_unique<value_t>(allocB, lcapB);
    EXPECT_EQ_U(resourceB.capacity(), cap_in_bytes(lcapB) * dash::size());
    EXPECT_EQ_U(reg.lookup(static_cast<dart_gptr_t>(p2.get())), &resourceB);
    auto *lp2 = dash::local_begin(p2.get(), dash::Team::All().myid());
    std::uninitialized_copy(std::begin(values), std::end(values), lp2);

    // If we swap pointers we also have to swap the memory resources...
    // Otherwise the allocators cannot free the underyling pointers
    LOG_MESSAGE("Swapping memory segments 1 and 2");
    std::swap(p1, p2);
    std::swap(resourceA, resourceB);

    // Verify values after swap...
    {
      EXPECT_EQ_U(resourceA.capacity(), cap_in_bytes(lcapB) * dash::size());
      EXPECT_EQ_U(resourceB.capacity(), cap_in_bytes(lcapA) * dash::size());

      /*
       * NOTES: Now, both the global and local iterators are invalidated and
       * dereferencing old local pointers or global references before the swap
       * operation is now undefined behavior.
       */

      // Here we pobtain local pointer to the local begin based on p2
      lp2 = dash::local_begin(p2.get(), dash::Team::All().myid());

      auto it =
          std::find_if(lp2, std::next(lp2, lcapA), [](const value_t val) {
            return val != value;
          });

      EXPECT_EQ_U(it, std::next(lp2, lcapA));

      lp1 = dash::local_begin(p1.get(), dash::Team::All().myid());
      EXPECT_TRUE_U(
          std::equal(lp1, std::next(lp1, lcapB), std::begin(values)));
    }
    // p2 goes out of scope -> deallocating underlying memory segment
  }

  /*
   * Move Semantics
   */
  using unique_ptr = decltype(p1);
  unique_ptr p2{};
  EXPECT_FALSE_U(p2.get());

  auto p1_gptr    = p1.get();
  auto p1_deleter = p1.get_deleter();

  // Move p1 into p2
  LOG_MESSAGE("Moving memory segment 1 into 2");
  p2 = std::move(p1);
  EXPECT_TRUE_U(p2.get());
  EXPECT_EQ_U(p2.get(), p1_gptr);
  EXPECT_FALSE_U(p1.get());
  EXPECT_EQ_U(p1.get(), nullptr);

  // compare the deleter, which is required for proper memory deallocation
  // This compares the underlying memory allocators and memory resources
  EXPECT_EQ_U(p2.get_deleter(), p1_deleter);

  EXPECT_EQ_U(
      reinterpret_cast<intptr_t>(
          reg.lookup(static_cast<dart_gptr_t>(p2.get()))),
      reinterpret_cast<intptr_t>(allocB.resource()));

  EXPECT_EQ_U(
      reinterpret_cast<intptr_t>(
          reg.lookup(static_cast<dart_gptr_t>(p2.get()))),
      reinterpret_cast<intptr_t>(&resourceB));

  auto *lp2 = dash::local_begin(p2.get(), dash::Team::All().myid());
  EXPECT_TRUE_U(std::equal(lp2, std::next(lp2, lcapB), std::begin(values)));

  memory_resource resource3{dash::Team::All()};
  allocator       alloc3{&resource3};
  auto            p3 = dash::allocate_unique<value_t>(alloc3, 15);
}

TEST_F(GlobStaticMemTest, HBWSpaceTest)
{
  using value_t     = int;
  using memory_t    = dash::GlobStaticMem<dash::HBWSpace>;
  using allocator_t = dash::GlobalAllocator<value_t, memory_t>;

  memory_t    memory{dash::Team::All()};
  allocator_t alloc{&memory};

  auto const gptr = alloc.allocate(10);

  EXPECT_TRUE_U(gptr);

  auto *lbegin = dash::local_begin(gptr, dash::Team::All().myid());
  auto *lend   = std::next(lbegin, 10);

  std::uninitialized_fill(lbegin, lend, dash::myid());

  ASSERT_EQ_U(*lbegin, dash::myid());

  alloc.deallocate(gptr, 10);
}

TEST_F(GlobStaticMemTest, CopyGlobPtr)
{
  using value_t   = int;
  using pointer_t = dash::GlobMemAllocPtr<value_t>;

  using memory_t    = dash::GlobStaticMem<dash::HostSpace>;
  using allocator_t = dash::GlobalAllocator<pointer_t, memory_t>;

  memory_t    memory{dash::Team::All()};
  allocator_t alloc{&memory};

  constexpr size_t nlelem = 10;

  // allocate from DART Buddy allocator
  auto gptr_memalloc = dash::memalloc<value_t>(nlelem);

  // initialize local memory allocated by dash::memalloc
  auto *lbegin = gptr_memalloc.local();
  EXPECT_TRUE_U(lbegin);
  std::iota(lbegin, std::next(lbegin, nlelem), dash::myid() * nlelem);

  /**
   *  Symmetric allocation of an array of pointers:
   *
   *  Each units stores allocates space for a single global pointer
   */

  // Note: implicit barrier
  constexpr size_t n_ptr = 1;
  auto gmem = alloc.allocate(n_ptr);

  // Copy pointer of local memalloc segment to neighbor
  auto right_neigh = (dash::myid() + 1) % dash::size();
  gmem[right_neigh] = gptr_memalloc;

  dash::barrier();


  //Obtain the first local element of the array
  auto *gmem_lbegin = dash::local_begin(gmem, dash::Team::All().myid());
  EXPECT_TRUE_U(gmem_lbegin);
  auto gmem_lbegin_ptrmemalloc = *gmem_lbegin;

  // verify...
  std::vector<value_t> exp(nlelem);
  auto left_neigh = (dash::myid() + dash::size() - 1) % dash::size();
  std::iota(std::begin(exp), std::end(exp), left_neigh * nlelem);

  for (std::size_t idx = 0; idx < nlelem; ++idx, gmem_lbegin_ptrmemalloc++) {
    auto value = static_cast<value_t>(*gmem_lbegin_ptrmemalloc);
    EXPECT_EQ_U(exp[idx], value);
  }

  dash::barrier();
}
