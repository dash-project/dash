
#include "CollectiveAllocatorTest.h"

#include <dash/allocator/CollectiveAllocator.h>


TEST_F(CollectiveAllocatorTest, Constructor)
{
  auto target           = dash::allocator::CollectiveAllocator<int>();
  dart_gptr_t requested = target.allocate(sizeof(int) * 10);

  ASSERT_EQ(0, requested.unitid);
}

