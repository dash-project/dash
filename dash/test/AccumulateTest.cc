
#include <gtest/gtest.h>

#include "AccumulateTest.h"
#include "TestBase.h"

#include <dash/Array.h>
#include <dash/algorithm/Accumulate.h>
#include <dash/algorithm/Fill.h>

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

TEST_F(AccumulateTest, StringConcatOperaton) {
  const size_t num_elem_local = 100;
  size_t num_elem_total       = _dash_size * num_elem_local;
  auto value = 2;
  
  // Create a vector
  dash::Array<int> target(4);
  target[0] = 1;
  target[1] = 2;
  target[2] = 3;
  target[3] = 4;
  
  dash::barrier();

  std::string result = std::accumulate(
				       std::next(target.begin()),
				       target.end(),
				       std::to_string(target[0]), // start element
                                    [](std::string x1, int x2) {
                                        return x1 + '-' + std::to_string(x2);
                                    });

  if(dash::myid() == 0) {
    ASSERT_STREQ("1-2-3-4", result.c_str());
  }
}
