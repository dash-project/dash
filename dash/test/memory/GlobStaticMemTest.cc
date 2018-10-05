#include "GlobStaticMemTest.h"
#include <dash/memory/MemorySpace.h>

#include <dash/std/memory.h>

TEST_F(GlobStaticMemTest, GlobalRandomAccess)
{
  auto globmem_local_elements = {1, 2, 3};
  using value_t = typename decltype(globmem_local_elements)::value_type;

  using memory_t = dash::GlobStaticMem<dash::HostSpace>;
  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;
  // local pointer type
  using lptr_t = typename std::pointer_traits<
      typename memory_t::local_void_pointer>::template rebind<value_t>;

  auto const nlelem = globmem_local_elements.size();
  auto const ngelem = dash::size() * nlelem;

  memory_t globmem{};

  globmem.allocate(
      globmem_local_elements.size() * sizeof(value_t), alignof(value_t));

  auto *lbegin = static_cast<lptr_t>(globmem.lbegin());

  std::uninitialized_copy(
      std::begin(globmem_local_elements),
      std::end(globmem_local_elements),
      lbegin);

  globmem.barrier();

  DASH_LOG_DEBUG_VAR("GlobStaticMemTest", globmem.capacity());
  EXPECT_EQ_U(globmem.capacity(), 3 * dash::size() * sizeof(value_t));

  if (dash::myid() == 0) {
    auto gbegin = static_cast<gptr_t>(globmem.begin());
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
        EXPECT_EQ(*gbegin, static_cast<gptr_t>(globmem.begin())[g]);
      }
      EXPECT_EQ(gbegin, static_cast<gptr_t>(globmem.begin()) + g);

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
    auto gbegin = static_cast<gptr_t>(globmem.begin());
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
      EXPECT_EQ(gend, static_cast<gptr_t>(globmem.begin()) + g);

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
}

TEST_F(GlobStaticMemTest, LocalBegin)
{
  auto target_local_elements = {1, 2, 3, 4};

  using value_t  = typename decltype(target_local_elements)::value_type;
  using memory_t = dash::GlobStaticMem<dash::HostSpace>;
  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;
  // local pointer type
  using lptr_t = typename std::pointer_traits<
      typename memory_t::local_void_pointer>::template rebind<value_t>;

  if (!dash::Team::All().is_leaf()) {
    SKIP_TEST_MSG("Team is already split");
  }

  auto &sub_team =
      dash::size() < 4 ? dash::Team::All() : dash::Team::All().split(2);

  auto target = memory_t(sub_team);
  auto gptr   = static_cast<gptr_t>(target.allocate(
      target_local_elements.size() * sizeof(value_t), alignof(value_t)));

  EXPECT_TRUE_U(gptr);

  auto *lbegin = static_cast<lptr_t>(target.lbegin());
  EXPECT_NE_U(lbegin, nullptr);

  std::uninitialized_copy(
      std::begin(target_local_elements),
      std::end(target_local_elements),
      lbegin);

  target.barrier();

  for (int l = 0; l < target_local_elements.size(); l++) {
    EXPECT_EQ_U(*(gptr + l), lbegin[l]);
  }
}

TEST_F(GlobStaticMemTest, MoveSemantics)
{
  using value_t  = int;
  using memory_t = dash::GlobStaticMem<dash::HostSpace>;
  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;
  // local pointer type
  using lptr_t = typename std::pointer_traits<
      typename memory_t::local_void_pointer>::template rebind<value_t>;
  // move construction
  {
    memory_t memory_a{};
    memory_a.allocate(5 * sizeof(value_t), alignof(value_t));

    *(static_cast<lptr_t>(memory_a.lbegin())) = 5;
    dash::barrier();

    memory_t memory_b(std::move(memory_a));
    int      value = *(static_cast<lptr_t>(memory_b.lbegin()));
    ASSERT_EQ_U(value, 5);
  }
  dash::barrier();
  // move assignment
  {
    memory_t memory_a{};
    memory_a.allocate(10 * sizeof(value_t), alignof(value_t));
    {
      memory_t memory_b{};
      memory_b.allocate(8 * sizeof(value_t), alignof(value_t));

      *(static_cast<lptr_t>(memory_a.lbegin())) = 1;
      *(static_cast<lptr_t>(memory_b.lbegin())) = 2;
      memory_a                                  = std::move(memory_b);
      // leave scope of memory_b
    }
    ASSERT_EQ_U(*(static_cast<lptr_t>(memory_a.lbegin())), 2);
  }
  dash::barrier();
  // swap
  {
    memory_t memory_a{};
    memory_a.allocate(10 * sizeof(value_t), alignof(value_t));
    memory_t memory_b{};
    memory_b.allocate(8 * sizeof(value_t), alignof(value_t));

    *(static_cast<lptr_t>(memory_a.lbegin())) = 1;
    *(static_cast<lptr_t>(memory_b.lbegin())) = 2;

    std::swap(memory_a, memory_b);
    ASSERT_EQ_U(*(static_cast<lptr_t>(memory_a.lbegin())), 2);
    ASSERT_EQ_U(*(static_cast<lptr_t>(memory_b.lbegin())), 1);
  }
}

TEST_F(GlobStaticMemTest, HBWSpaceTest)
{
  using value_t  = int;
  using memory_t = dash::GlobStaticMem<dash::HBWSpace>;

  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;
  // local pointer type
  using lptr_t = typename std::pointer_traits<
      typename memory_t::local_void_pointer>::template rebind<value_t>;

  memory_t memory{};
  auto     gptr = static_cast<gptr_t>(
      memory.allocate(10 * sizeof(value_t), alignof(value_t)));

  EXPECT_TRUE_U(gptr);

  auto *lbegin = static_cast<lptr_t>(memory.lbegin());
  auto *lend   = static_cast<lptr_t>(memory.lend());
  EXPECT_EQ_U(lend, std::next(lbegin, 10));
  std::uninitialized_fill(lbegin, lend, dash::myid());

  ASSERT_EQ_U(*lbegin, dash::myid());
}

TEST_F(GlobStaticMemTest, MakeUnique)
{
  using value_t  = int;
  using memory_t = dash::GlobStaticMem<dash::HBWSpace>;

  // global pointer type
  using gptr_t = typename std::pointer_traits<
      typename memory_t::void_pointer>::template rebind<value_t>;

  memory_t globmem{};

  //create a global pointer to an array of 10 integers
  auto ptr = dash::make_unique<value_t>(&globmem, 10);

  EXPECT_TRUE_U(ptr);

  static_assert(
      std::is_same<decltype(ptr.get()), gptr_t>::value,
      "invalid pointer type");

  auto& reg = dash::internal::MemorySpaceRegistry::GetInstance();
  EXPECT_EQ_U(reg.lookup(static_cast<dart_gptr_t>(ptr.get())), &globmem);

  ptr.reset();

  EXPECT_EQ_U(reg.lookup(static_cast<dart_gptr_t>(ptr.get())), nullptr);

  //we can compare both unique_ptr and dash::GlobPtr with a nullptr_t
  EXPECT_EQ_U(ptr, nullptr);
  EXPECT_EQ_U(ptr.get(), nullptr);
  EXPECT_FALSE_U(ptr);
  EXPECT_FALSE_U(ptr.get());
}
