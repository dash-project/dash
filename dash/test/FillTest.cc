#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "FillTest.h"

#include <array>

TEST_F(FillTest, TestAllItemsFilled)
{
  typedef typename Array_t::value_type value_t;

  DASH_LOG_TRACE("FillTest.TestAllItemsFilled", "allocate array");
  // Initialize global array:
  Array_t array(_num_elem * dash::size());
  // arbitrary value
  value_t val = 17;
  // Fill array with value
  DASH_LOG_TRACE("FillTest.TestAllItemsFilled", "fill array");
  dash::fill(array.begin(), array.end(), val);
  // Wait for all units
  array.barrier();
  // Local range in array:
  auto lbegin = array.lbegin();
  auto lend   = array.lend();
  DASH_LOG_TRACE("FillTest.TestAllItemsFilled", "local range of array");
  auto lrange = dash::local_range(array.begin(), array.end());
  EXPECT_EQ_U(lbegin, lrange.begin);
  EXPECT_EQ_U(lend,   lrange.end);
  EXPECT_EQ_U(array.pattern().local_size(), lend - lbegin);

  for(int l = 0; lbegin != lend; ++lbegin, l++)
  {
    DASH_LOG_TRACE("FillTest.TestAllItemsFilled", "test lbegin +", l);
    EXPECT_EQ_U(17, static_cast<value_t>(*lbegin));
  }
}
