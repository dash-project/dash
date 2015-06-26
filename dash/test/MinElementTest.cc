#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "MinElementTest.h"

TEST_F(MinElementTest, TestFindCenter)
{
  Element_t min_value = 11;
  // Initialize global array:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 41;
      LOG_MESSAGE("Setting array[%d] = %d",
                  i, value);
      array[i] = value;
    }
    // Set minimum element in the center position:
    index_t min_pos = array.size() / 2;
    LOG_MESSAGE("Setting array[%d] = %d (min)", 
                min_pos, min_value);
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  // Run min_element on complete array
  dash::GlobPtr<Element_t> found_gptr =
    dash::min_element(array.begin(), array.end());
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  // Check minimum value found
  EXPECT_EQ(min_value, found_min);
}
