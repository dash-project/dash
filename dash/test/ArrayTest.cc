#include <libdash.h>
#include "ArrayTest.h"

TEST_F(ArrayTest, SingleWriteMultipleRead) {
  // Create array instances using varying constructor options
  dash::Array<int> arr1(_num_elem * _dash_size);
  dash::Array<int> arr2(_num_elem * _dash_size,
                        dash::BLOCKED);
  dash::Array<int> arr3(_num_elem * _dash_size,
                        dash::Team::All());
  dash::Array<int> arr4(_num_elem * _dash_size,
                        dash::BLOCKED,
                        dash::Team::All());
  dash::Pattern<1> pat(_num_elem * _dash_size);
  dash::Array<int> arr5(pat);
  // Fill arrays with incrementing values
  if(_dash_id == 0) {
    for(int i = 0; i < arr1.size(); i++) {
      arr1[i] = i;
      arr2[i] = i;
      arr3[i] = i;
      arr4[i] = i;
      arr5[i] = i;
    }
  }
  // Wait for teams
  dash::Team::All().barrier();
  // Read and assert values in arrays
  for(int i = 0; i < arr1.size(); i++) {
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr2[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr3[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr4[i]));
    EXPECT_EQ(static_cast<int>(arr1[i]), static_cast<int>(arr5[i]));
  }
}

