#ifndef DASH__TEST__SIMPLE_MEMORY_POOL_RESOURCE_TEST_H__INCLUDED
#define DASH__TEST__SIMPLE_MEMORY_POOL_RESOURCE_TEST_H__INCLUDED

#include "../TestBase.h"

#include <dash/memory/SimpleMemoryPoolResource.h>
#include <dash/Exception.h>

class SimpleMemoryPoolTest : public dash::test::TestBase {
 protected:
  SimpleMemoryPoolTest() {}
  virtual ~SimpleMemoryPoolTest() {}
};

/**
 * Test fixture for class for SimpleMemoryPoolTest
 */
template <typename ValueType, class LocalMemorySpace = dash::HostSpace>
class Stack {
  struct Node {
    ValueType d_value;  // payload value
    Node *d_next_p;     // pointer to the next node
  };

  using memory_traits = dash::memory_space_traits<LocalMemorySpace>;

  using MemoryPool = dash::SimpleMemoryPoolResource<LocalMemorySpace>;


 private:
  // DATA
   Node *                       m_head_p;  // pointer to the first node
   int                          m_size;    // size of the stack
   MemoryPool m_pool;    // memory manager for the stack

 public:
  // CREATORS
   Stack(
       LocalMemorySpace *resource = static_cast<LocalMemorySpace *>(
           dash::get_default_local_memory_space<
               typename memory_traits::memory_space_type_category>()));

   // MANIPULATORS
   void push(int value);

   void pop();

   // ACCESSORS
   int top();

   std::size_t size();
};

// CREATORS
template <typename ValueType, class LocalMemorySpace>
Stack<ValueType, LocalMemorySpace>::Stack(LocalMemorySpace * resource)
  : m_head_p(0)
  , m_size(0)
  , m_pool(resource)
{
}
//
// MANIPULATORS
template <typename ValueType, class LocalMemorySpace>
void Stack<ValueType, LocalMemorySpace>::push(int value)
{
  Node *newNode =
      static_cast<Node *>(m_pool.allocate(sizeof(Node), alignof(Node)));
  //
  newNode->d_value = value;
  newNode->d_next_p = m_head_p;
  m_head_p = newNode;
  //
  ++m_size;
}
//
template <typename ValueType, class LocalMemorySpace>
void Stack<ValueType, LocalMemorySpace>::pop()
{
  DASH_ASSERT(0 != size());
  //
  Node *n = m_head_p;
  m_head_p = m_head_p->d_next_p;
  m_pool.deallocate(n, sizeof(Node), alignof(Node));
  --m_size;
}
//
// ACCESSORS
template <typename ValueType, class LocalMemorySpace>
int Stack<ValueType, LocalMemorySpace>::top()
{
  DASH_ASSERT(0 != size());
  //
  return m_head_p->d_value;
}
//
template <typename ValueType, class LocalMemorySpace>
std::size_t Stack<ValueType, LocalMemorySpace>::size()
{
  return m_size;
}
//..

#endif  // DASH__TEST__SIMPLE_MEMORY_POOL_RESOURCE_TEST_H__INCLUDED
