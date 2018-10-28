#include "SimpleMemoryPoolResourceTest.h"

#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>


TEST_F(SimpleMemoryPoolTest, usageExampleStack)
{
  DASH_TEST_LOCAL_ONLY();

  using ValueType = int;
  using IntStack = Stack<ValueType, dash::HostSpace>;

  IntStack stack{};
  stack.push(1);
  stack.push(20);
  stack.push(300);
  stack.push(4000);
  stack.push(50000);

  ASSERT_EQ(stack.size(), 5);
  ASSERT_EQ(stack.top(), 50000);
  stack.pop();
  ASSERT_EQ(stack.size(), 4);
  ASSERT_EQ(stack.top(), 4000);
  stack.pop();
  ASSERT_EQ(stack.size(), 3);
  ASSERT_EQ(stack.top(), 300);
  stack.pop();
  ASSERT_EQ(stack.size(), 2);
  ASSERT_EQ(stack.top(), 20);
  stack.pop();
  ASSERT_EQ(stack.size(), 1);
  ASSERT_EQ(stack.top(), 1);
  stack.pop();

  ASSERT_EQ(stack.size(), 0);
}

