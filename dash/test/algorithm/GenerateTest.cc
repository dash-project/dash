
#include "GenerateTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/LocalRange.h>


TEST_F(GenerateTest, TestGenerate)
{
  typedef typename Array_t::value_type value_t;

  // Initialize global array:
  Array_t array(_num_elem);
  // Generator function
  std::function<Element_t()> f = [](){ return 17L; };
  // Fill Array with given generator function
  dash::generate(array.begin(), array.end(), f);
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

TEST_F(GenerateTest, TestGenerateWithIndex)
{
  typedef typename Array_t::value_type value_t;

  // Initialize global array:
  Array_t array(_num_elem);
  // Generator function
  auto f = [](Array_t::index_type idx){ return 2*idx; };
  // Fill Array with given generator function
  dash::generate_with_index(array.begin(), array.end(), f);
  // Wait for all units
  array.barrier();

  // check global index range
  if (dash::myid() == 0) {
    for (size_t idx = 0;
         idx != array.size();
         ++idx) {
      ASSERT_EQ_U(idx * 2.0, array[idx]);
    }
  }
}
