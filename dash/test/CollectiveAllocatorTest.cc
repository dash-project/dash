#include <dash/Allocator.h>
#include "CollectiveAllocatorTest.h"
#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"

TEST_F(CollectiveAllocatorTest, Constructor)
{
  dash::allocator::CollectiveAllocator<int> target = dash::allocator::CollectiveAllocator<int>();

  dart_gptr_t requested = target.allocate(sizeof(int) * 10);

  ASSERT_EQ(dash::myid(), requested.unitid);
}

