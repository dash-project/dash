#include <libdash.h>
#include <gtest/gtest.h>
#include <functional>

#include "TestBase.h"
#include "ForEachTest.h"

TEST_F(ForEachTest, TestArrayAllInvoked)
{
  std::function<void(index_t)> invoke =
    std::bind(&ForEachTest::count_invoke, this, std::placeholders::_1);

  Array_t array(_num_elem);
  // Run for_each on complete array
  dash::for_each(array.begin(), array.end(), invoke);
  // Should have been invoked on every local index in the array:
  EXPECT_EQ(array.lsize(), _invoked_indices.size());
  // Wait for all units
  array.barrier();
}
