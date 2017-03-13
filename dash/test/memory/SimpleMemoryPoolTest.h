#ifndef DASH__TEST__UNIT_ID_TEST_H__INCLUDED
#define DASH__TEST__UNIT_ID_TEST_H__INCLUDED

#include "../TestBase.h"

#include <dash/memory/SimpleMemoryPool.h>
#include <dash/Exception.h>

/**
 * Test fixture for class DASH unit id types.
 */
class SimpleMemoryPoolTest : public dash::test::TestBase {
 protected:
  SimpleMemoryPoolTest() {}
  virtual ~SimpleMemoryPoolTest() {}
};

template <typename ValueType, class Alloc = std::allocator<ValueType>>
class Stack {
  struct Node {
    ValueType d_value;  // payload value
    Node *d_next_p;     // pointer to the next node
  };

  using MemoryPool = dash::SimpleMemoryPool<Node, Alloc>;

 private:
  // DATA
  Node *m_head_p;     // pointer to the first node
  int m_size;         // size of the stack
  MemoryPool m_pool;  // memory manager for the stack
                      //
 public:
  // CREATORS
  Stack(const Alloc & allocator = Alloc());
  // MANIPULATORS
  void push(int value);

  void pop();

  // ACCESSORS
  int top();

  std::size_t size();
};

// CREATORS
template <typename ValueType, class Alloc>
Stack<ValueType, Alloc>::Stack(const Alloc &allocator)
  : m_head_p(0)
  , m_size(0)
  , m_pool(allocator)
{
}
//
// MANIPULATORS
template <typename ValueType, class Alloc>
void Stack<ValueType, Alloc>::push(int value)
{
  Node *newNode = m_pool.allocate();
  //
  newNode->d_value = value;
  newNode->d_next_p = m_head_p;
  m_head_p = newNode;
  //
  ++m_size;
}
//
template <typename ValueType, class Alloc>
void Stack<ValueType, Alloc>::pop()
{
  DASH_ASSERT(0 != size());
  //
  Node *n = m_head_p;
  m_head_p = m_head_p->d_next_p;
  m_pool.deallocate(n);
  --m_size;
}
//
// ACCESSORS
template <typename ValueType, class Alloc>
int Stack<ValueType, Alloc>::top()
{
  DASH_ASSERT(0 != size());
  //
  return m_head_p->d_value;
}
//
template <typename ValueType, class Alloc>
std::size_t Stack<ValueType, Alloc>::size()
{
  return m_size;
}
//..

#endif  // DASH__TEST__UNIT_ID_TEST_H__INCLUDED
