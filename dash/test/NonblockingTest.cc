#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "NonblockingTest.h"

TEST_F(NonblockingTest, GlobAsyncRef) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(_dash_size * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = _dash_id;
  }
  array.barrier();
  // Test global async references on array elements:
  dash::GlobAsyncRef<int> gar_local(&array.local[0]);
  ASSERT_EQ_U(true, gar_local.is_local());
}
