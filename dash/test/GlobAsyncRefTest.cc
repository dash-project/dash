
#include "GlobAsyncRefTest.h"

#include <dash/GlobAsyncRef.h>
#include <dash/Array.h>


TEST_F(GlobAsyncRefTest, IsLocal) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(dash::size() * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();
  // Test global async references on array elements:
  dash::GlobAsyncRef<int> gar_local_l(&array.local[0]);
  ASSERT_EQ_U(true, gar_local_l.is_local());
  // Test global async references on array elements:
  auto global_offset      = array.pattern().global(0);
  // Reference first local element in global memory:
  dash::GlobRef<int> gref = array[global_offset];
  dash::GlobAsyncRef<int> gar_local_g(gref);
  ASSERT_EQ_U(true, gar_local_g.is_local());
}

/**
 * Non-blocking writes to distributed array with push semantics.
 */
TEST_F(GlobAsyncRefTest, Push) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(dash::size() * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();
  // Assign values asynchronously:
  for (auto gi = 0; gi < array.size(); ++gi) {
    if (array[gi].is_local()) {
      // Changes local value only
      array.async[gi]++;
    }
  }
  // Flush local window:
  array.async.push();
  // Test values in local window. Changes by all units should be visible:
  for (auto li = 0; li < array.lcapacity(); ++li) {
    // All local values incremented once by all units
    ASSERT_EQ_U(dash::myid().id + 1,
                array.local[li]);
  }
}

