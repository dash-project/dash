#include "TestBase.h"
#include "ArrayLargeStructTest.h"

#include <dash/Array.h>

using namespace std;

TEST_F(ArrayLargeStruct, LocalArrayTest)
{
  size_t array_size = _dash_size;
  // Create array instances using varying constructor options
  LOG_MESSAGE("Array size: %d", array_size);
  // Initialize arrays
  LOG_MESSAGE("Initialize arr1");
  dash::Array<DGNode> arr1(array_size);
  // Check array sizes
  ASSERT_EQ(array_size, arr1.size());
  // Fill arrays with incrementing values

  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning array values");
    DGNode *write = new DGNode();
    DGNode *read = new DGNode();

    for(size_t i = 0; i < array_size; ++i) {
      write->len = 10000;
      //Blocking Write
      arr1[i].put(write);
      //Blocking Read
      arr1[i].get(read);
      ASSERT_EQ(read->len, 10000);
    }
    delete write;
    delete read;
  }

  // Units waiting for value initialization
  arr1.barrier();
}

