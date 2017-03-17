#include "SimpleMemoryPoolTest.h"


#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>


TEST_F(SimpleMemoryPoolTest, usageExampleStack)
{
  DASH_TEST_LOCAL_ONLY();

  using ValueType = int;
  using Alloc = std::allocator<ValueType>;
  using IntStack = Stack<ValueType, Alloc>;

  int wait = 0;
  while(wait);

  IntStack stack{};
  stack.push(1);
  stack.push(20);
  stack.push(300);
  stack.push(4000);
  stack.push(50000);

  ASSERT_EQ(stack.size(), 5);
  stack.pop();
  ASSERT_EQ(stack.size(), 4);
  stack.pop();
  ASSERT_EQ(stack.size(), 3);
  stack.pop();
  ASSERT_EQ(stack.size(), 2);
  stack.pop();
  ASSERT_EQ(stack.size(), 1);
  stack.pop();

  ASSERT_EQ(stack.size(), 0);
}

