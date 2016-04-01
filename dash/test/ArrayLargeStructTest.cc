#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "ArrayLargeStructTest.h"
#include <iostream>

using namespace std;

void foo(dash::Array<DGNode>& nodes)
{
  cout << "size of nodes is " << nodes.size() << endl;
}

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

  //int DebugWait = 1;
  //cout << "ProcessId " << getpid();
  //while (DebugWait);

  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning array values");
    DGNode *node;
    for(size_t i = 0; i < array_size; ++i) {
      node = new DGNode();
      //DGNode node;
      node->len = 10000;
      //node.val[10] = 1;
      arr1[i] = *node;
    }
  }

  // Units waiting for value initialization
  arr1.barrier();

  foo(arr1);
}

