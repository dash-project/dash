#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "ArrayTest.h"

TEST_F(ArrayTest, SingleWriteMultipleRead) {
  // Create array instances using varying constructor options
  LOG_MESSAGE("arr1");
  dash::Array<int> arr1(_num_elem * _dash_size);
  LOG_MESSAGE("arr2");
  dash::Array<int> arr2(_num_elem * _dash_size,
                        dash::BLOCKED);
  LOG_MESSAGE("arr3");
  dash::Array<int> arr3(_num_elem * _dash_size,
                        dash::Team::All());
  LOG_MESSAGE("arr4");
  dash::Array<int> arr4(_num_elem * _dash_size,
                        dash::CYCLIC,
                        dash::Team::All());
  LOG_MESSAGE("arr5");
  dash::Array<int> arr5(_num_elem * _dash_size,
                        dash::BLOCKCYCLIC(12));
  dash::Pattern<1> pat(_num_elem * _dash_size);
  LOG_MESSAGE("arr6");
  dash::Array<int> arr6(pat);

  return;

  // Fill arrays with incrementing values
  if(_dash_id == 0) {
    for(int i = 0; i < arr1.size(); i++) {
      LOG_MESSAGE("Assigning arr*[%d]", i);
      arr1[i] = i;
      arr2[i] = i;
      arr3[i] = i;
      arr4[i] = i;
      arr5[i] = i;
      arr6[i] = i;
    }
  }
  LOG_MESSAGE("Team barrier ...");
  // Wait for teams
  dash::Team::All().barrier();
  // Read and assert values in arrays
  for(int i = 0; i < arr1.size(); i++) {
    LOG_MESSAGE("Checking arr*[%d]", i);
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr2[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr3[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr4[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr5[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr6[i]));
  }
}

