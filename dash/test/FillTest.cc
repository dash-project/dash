#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "FillTest.h"

#include <array>

TEST_F(FillTest, ArrayLocalPlusLocal)
{
  // Initialize global array:
  Array_t array(_num_elem);
  // arbitrary value
  Element_t val = 17L;
  // Fill array with value
  dash::fill(array.begin(), array.end(), val);
  // Wait for all units
  array.barrier();
  // Local begin
  auto lbegin = array.lbegin();
  // Local end
  auto lend = array.lend();

  for(; lbegin != lend; ++lbegin)
  {
    EXPECT_EQ(*lbegin, *lend);
  }
}
