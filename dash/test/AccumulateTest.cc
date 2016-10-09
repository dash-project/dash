#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "AccumulateTest.h"

#include <array>

TEST_F(AccumulateTest, SimpleConstructor) {
  const size_t num_elem_local = 100;
  size_t num_elem_total       = _dash_size * num_elem_local;
  auto value = 2;
  
  dash::Array<int> target(num_elem_total, dash::BLOCKED);

  dash::fill(target.begin(), target.end(), value);
  
  dash::barrier();

  int result = dash::accumulate(target.begin(),
				target.end(),
				0); //start value			      

  if(dash::myid() == 0) {
    ASSERT_EQ_U(num_elem_total * value, result);
  }
}
