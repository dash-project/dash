#include <libdash.h>
#include <gtest/gtest.h>

#include <limits>
#include <algorithm>

#include "TestBase.h"
#include "MismatchTest.h"

TEST_F(MismatchTest, TestSimpleMismatch)
{
  typedef int Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t _num_elem               = dash::Team::All().size();
  Element_t init_fill     = 0;
  Element_t mismatch_fill = 1;

  //Initialize global array and fill it with init_fill:
  Array_t array_1(_num_elem);
  if (dash::myid()==0) {
    for (size_t i = 0; i < array_1.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill = ", i, init_fill);
      array_1[i] = init_fill;
    }
  }
  array_1.barrier();

  // Copy array and modify it
  Array_t array_2(_num_elem);
  dash::copy(array_1.begin(), array_1.end(),
             array_2.begin());

  array_2.barrier();

  Element_t mismatch_t = _num_elem / 2;
  array_2[mismatch_t]  = mismatch_fill;
  array_2.barrier();

  LOG_MESSAGE("Finished initialization of two arrays");

  //Check dash::algorithm::mismatch
  LOG_MESSAGE("Start test of mismatch");
  auto test_result = dash::mismatch(array_1.begin(), array_1.end(),
                                    array_2.begin(), array_2.end(),
                                    std::equal_to<Element_t>());

  EXPECT_NE_U(test_result.first, test_result.second);
}

