
#include "FillTest.h"

#include <dash/Array.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Fill.h>

#include <array>


TEST_F(FillTest, TestAllItemsFilled)
{
  typedef double                                      Element_t;
  typedef dash::Array<Element_t>                        Array_t;
  typedef typename Array_t::pattern_type::index_type    index_t;
  typedef typename Array_t::value_type                  value_t;

  /// Using a prime to cause inconvenient strides
  size_t num_local_elem = 513;

  LOG_MESSAGE("FillTest.TestAllItemsFilled: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());
  // arbitrary value
  value_t val = 17;
  // Fill array with value
  LOG_MESSAGE("FillTest.TestAllItemsFilled: fill array");
  dash::fill(array.begin(), array.end(), val);
  // Wait for all units
  array.barrier();
  // Local range in array:
  auto lbegin = array.lbegin();
  auto lend   = array.lend();
  LOG_MESSAGE("FillTest.TestAllItemsFilled: local range of array");
  auto lrange = dash::local_range(array.begin(), array.end());
  EXPECT_EQ_U(lbegin, lrange.begin);
  EXPECT_EQ_U(lend,   lrange.end);
  EXPECT_EQ_U(array.pattern().local_size(), lend - lbegin);

  for(int l = 0; lbegin != lend; ++lbegin, l++)
  {
    EXPECT_EQ_U(17, static_cast<value_t>(*lbegin));
  }
}
