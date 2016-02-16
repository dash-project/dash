#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "FillTest.h"

#include <array>

TEST_F(FillTest, TestAllItemsFilled)
{
  typedef typename Array_t::value_type value_t;

  // Initialize global array:
  Array_t array(_num_elem);
  // arbitrary value
  Element_t val = 17L;
  // Fill array with value
  dash::fill(array.begin(), array.end(), val);
  // Wait for all units
  array.barrier();
  // Local range in array:
  auto lbegin = array.lbegin();
  auto lend   = array.lend();
  auto lrange = dash::local_range(array.begin(), array.end());
  ASSERT_EQ_U(lbegin, lrange.begin);
  ASSERT_EQ_U(lend,   lrange.end);
  ASSERT_EQ_U(array.pattern().local_size(), lend - lbegin);

  for(; lbegin != lend; ++lbegin)
  {
    ASSERT_EQ_U(17, static_cast<value_t>(*lbegin));
  }
}
