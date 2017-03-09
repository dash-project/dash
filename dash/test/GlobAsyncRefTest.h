#ifndef DASH__TEST__GLOB_ASYNC_REF_TEST_H_
#define DASH__TEST__GLOB_ASYNC_REF_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for non-blocking operations using \c dash::GlobAsyncRef.
 */
class GlobAsyncRefTest : public dash::test::TestBase {
protected:

  GlobAsyncRefTest() {
    LOG_MESSAGE(">>> Test suite: GlobAsyncRefTest");
  }

  virtual ~GlobAsyncRefTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobAsyncRefTest");
  }
};

#endif // DASH__TEST__GLOB_ASYNC_REF_TEST_H_
