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
  dash::GlobAsyncRef<int> gar_local_l(&array.local[0]);
  ASSERT_EQ_U(true, gar_local_l.is_local());
  // Test global async references on array elements:
  auto global_offset      = array.pattern().local_to_global_index(0);
  // Reference first local element in global memory:
  dash::GlobRef<int> gref = array[global_offset];
  dash::GlobAsyncRef<int> gar_local_g(gref);
  ASSERT_EQ_U(true, gar_local_g.is_local());
}

TEST_F(NonblockingTest, ArrayBulkWrite) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(_dash_size * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = _dash_id;
  }
  array.barrier();
  // Assign values asynchronously:
  for (auto gi = 0; gi < array.size(); ++gi) {
    if (array[gi].is_local()) {
      array.async[gi]++;
    }
  }
  // Flush local window:
  array.async.flush_local_all();
// array.barrier();
  // Test values in local window. Changes by all units should be visible:
  for (auto li = 0; li < array.lcapacity(); ++li) {
    // All local values incremented once by all units
    ASSERT_EQ_U(_dash_id + 1,
                array.local[li]);
  }
}
