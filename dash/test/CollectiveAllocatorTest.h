#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::CollectiveAllocator
 */
class CollectiveAllocatorTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;
  int    _num_elem;
  
  CollectiveAllocatorTest()
  : _dash_id(0),
    _dash_size(0),
    _num_elem(0) {
    LOG_MESSAGE(">>> Test suite: CollectiveAllocatorTest");
  }

  virtual ~CollectiveAllocatorTest() {
    LOG_MESSAGE("<<< Closing test suite: CollectiveAllocatorTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
