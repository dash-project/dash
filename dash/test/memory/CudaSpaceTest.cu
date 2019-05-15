#include "GlobStaticMemTest.h"

#include <dash/GlobRef.h>
#include <dash/allocator/GlobalAllocator.h>
#include <dash/memory/UniquePtr.h>
#include <dash/memory/CudaSpace.h>

TEST_F(GlobStaticMemTest, CudaSpace)
{
  using value_t     = int;
  using memory_t    = dash::GlobStaticMem<dash::CudaSpace>;
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
