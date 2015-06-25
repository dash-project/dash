#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "ArrayTest.h"

TEST_F(ArrayTest, SingleWriteMultipleRead) {
  dart_unit_t myid  = dash::myid();
  size_t array_size = _num_elem * _dash_size;
  // Create array instances using varying constructor options
  LOG_MESSAGE("Array size: %d", array_size);
  try {
    // Initialize arrays
    LOG_MESSAGE("Initialize arr1");
    dash::Array<int> arr1(array_size);
    LOG_MESSAGE("Initialize arr2");
    dash::Array<int> arr2(array_size,
                          dash::BLOCKED);
    LOG_MESSAGE("Initialize arr3");
    dash::Array<int> arr3(array_size,
                          dash::Team::All());
    LOG_MESSAGE("Initialize arr4");
    dash::Array<int> arr4(array_size,
                          dash::CYCLIC,
                          dash::Team::All());
    LOG_MESSAGE("Initialize arr5");
    dash::Array<int> arr5(array_size,
                          dash::BLOCKCYCLIC(12));
    LOG_MESSAGE("Initialize arr6");
    dash::Pattern<1> pat(array_size);
    dash::Array<int> arr6(pat);
    // Check array sizes
    ASSERT_EQ(array_size, arr1.size());
    ASSERT_EQ(array_size, arr2.size());
    ASSERT_EQ(array_size, arr3.size());
    ASSERT_EQ(array_size, arr4.size());
    ASSERT_EQ(array_size, arr5.size());
    ASSERT_EQ(array_size, arr6.size());
    // Fill arrays with incrementing values
    if(_dash_id == 0) {
      LOG_MESSAGE("Assigning array values");
      for(int i = 0; i < arr1.size(); i++) {
        arr1[i] = i;
        arr2[i] = i;
        arr3[i] = i;
        arr4[i] = i;
        arr5[i] = i;
        arr6[i] = i;
      }
    }
    // Units waiting for value initialization
    dash::Team::All().barrier();
    // Read and assert values in arrays
    for(int i = 0; i < arr1.size(); i++) {
      LOG_MESSAGE("Checking arr1[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr1[i]));
      LOG_MESSAGE("Checking arr2[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr2[i]));
      LOG_MESSAGE("Checking arr3[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr3[i]));
      LOG_MESSAGE("Checking arr4[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr4[i]));
      LOG_MESSAGE("Checking arr5[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr5[i]));
      LOG_MESSAGE("Checking arr6[%d]", i);
      ASSERT_EQ_U(i, static_cast<int>(arr6[i]));
    }
  } catch (dash::exception::InvalidArgument & ia) {
    LOG_MESSAGE("ERROR: %s", ia.what());
    ASSERT_FAIL();
  }
}

