
#include "CollectiveAllocatorTest.h"

#include <dash/allocator/CollectiveAllocator.h>


TEST_F(CollectiveAllocatorTest, Constructor)
{
  auto target           = dash::allocator::CollectiveAllocator<int>();
  dart_gptr_t requested = target.allocate(sizeof(int) * 10);

  ASSERT_EQ(0, requested.unitid);
}

TEST_F(CollectiveAllocatorTest, TeamAlloc)
{
  if (_dash_size < 2) {
    SKIP_TEST_MSG("Test case requires at least two units");
  }
  dash::Team& subteam   = dash::Team::All().split(2);

  auto target           = dash::allocator::CollectiveAllocator<int>(subteam);
  dart_gptr_t requested = target.allocate(sizeof(int) * 10);

  // make sure the unitid in the gptr is
  // team-local and 0 instead of the corresponding global unit ID
  ASSERT_EQ(0, requested.unitid);
}

