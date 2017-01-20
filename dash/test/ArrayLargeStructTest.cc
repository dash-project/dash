
#include <gtest/gtest.h>

#include "ArrayLargeStructTest.h"
#include "TestBase.h"

#include <dash/Array.h>

#include <iostream>
#include <memory>


using namespace std;

TEST_F(ArrayLargeStruct, LocalArrayTest)
{
  size_t array_size = _dash_size;
  // Create array instances using varying constructor options
  LOG_MESSAGE("Array size: %zu", array_size);
  // Initialize arrays
  LOG_MESSAGE("Initialize arr1");
  dash::Array<DGNode> arr1(array_size);
  // Check array sizes
  ASSERT_EQ(array_size, arr1.size());
  // Fill arrays with incrementing values

  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning array values");
    auto write = std::unique_ptr<DGNode>(new DGNode());
    auto read  = std::unique_ptr<DGNode>(new DGNode());

    for(size_t i = 0; i < array_size; ++i) {
      write->len = 10000;
      //Blocking Write
      arr1[i].put(write.get());
      //Blocking Read
      arr1[i].get(read.get());
      ASSERT_EQ(read->len, 10000);
    }
  }

  // Units waiting for value initialization
  arr1.barrier();
}

