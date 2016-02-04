#include <libdash.h>
#include <gtest/gtest.h>
#include <functional>

#include "TestBase.h"
#include "GenerateTest.h"

TEST_F(GenerateTest, TestAllItemsGenerated)
{
  // Initialize global array:
  Array_t array(_num_elem);
  // Generator function
  std::function<Element_t()> f = [](){ return 17L; };
  // Fill Array with given generator function
  dash::generate(array.begin(), array.end(), f);
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
