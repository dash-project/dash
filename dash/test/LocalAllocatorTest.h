#ifndef DASH__TEST__LOCALALLOC_TEST_H_
#define DASH__TEST__LOCALALLOC_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::LocalAllocator
 */
class LocalAllocatorTest : public dash::test::TestBase {
protected:
  LocalAllocatorTest() {
    LOG_MESSAGE(">>> Test suite: LocalAllocatorTest");
  }

  virtual ~LocalAllocatorTest() {
    LOG_MESSAGE("<<< Closing test suite: CollectiveAllocatorTest");
  }
};

#endif // DASH__TEST__LOCALALLOC_TEST_H_
