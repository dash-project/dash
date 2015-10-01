
#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "CopyTest.h"

TEST_F(CopyTest, GlobalToLocalBlocking) {
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int local_copy[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  dash::copy(array.begin(),
             array.begin() + num_elem_per_unit,
             local_copy);
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    ASSERT_EQ_U(static_cast<int>(array[l]),
                local_copy[l]);
  }
}

