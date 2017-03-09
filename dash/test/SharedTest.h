#ifndef DASH__TEST__SHARED_TEST_H_
#define DASH__TEST__SHARED_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Shared
 */
class SharedTest : public dash::test::TestBase {
protected:

  SharedTest() {
    LOG_MESSAGE(">>> Test suite: SharedTest");
  }

  virtual ~SharedTest()
  {
    LOG_MESSAGE("<<< Closing test suite: SharedTest");
  }
};

#endif // DASH__TEST__SHARED_TEST_H_
