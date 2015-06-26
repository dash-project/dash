#include <libdash.h>
#include <gtest/gtest.h>
#include <functional>

#include "TestBase.h"
#include "ForEachTest.h"

TEST_F(ForEachTest, TestArrayAllInvoked)
{
  // Shared variable for total number of invoked callbacks:
  dash::Shared<size_t> num_invoked_indices;
  // Create for_each callback from member function:
  std::function<void(index_t)> invoke =
    std::bind(&ForEachTest::count_invoke, this, std::placeholders::_1);
  // Initialize callback counter:
  if (dash::myid() == 0) {
    num_invoked_indices.set(0);
  }
  // Ensure value global counter is published to all units
  dash::Team::All().barrier();
  // Initialize global array:
  Array_t array(_num_elem);
  // Run for_each on complete array
  dash::for_each(array.begin(), array.end(), invoke);
  // Should have been invoked on every local index in the array:
  LOG_MESSAGE("Local number of inspected indices: %d",
    _invoked_indices.size());
  EXPECT_EQ(array.lsize(), _invoked_indices.size());
  // TODO: Not exactly an atomic increment:
  // atomic {
  size_t num_invoked_indices_cur = num_invoked_indices.get();
  num_invoked_indices.set(
    num_invoked_indices_cur + _invoked_indices.size());
  // }
  // Wait for all units
  array.barrier();
  size_t num_invoked_indices_all = num_invoked_indices.get();
  LOG_MESSAGE("Total number of inspected indices: %d",
    num_invoked_indices_all);
  EXPECT_EQ(_num_elem, num_invoked_indices_all);
}
