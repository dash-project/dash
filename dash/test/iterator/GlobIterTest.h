#ifndef DASH__TEST__GLOB_ITER_REF_TEST_H_
#define DASH__TEST__GLOB_ITER_REF_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for non-blocking operations using \c dash::GlobAsyncRef.
 */
class GlobIterTest : public dash::test::TestBase {
protected:

  GlobIterTest() {
    LOG_MESSAGE(">>> Test suite: GlobIterTest");
  }

  virtual ~GlobIterTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobIterTest");
  }
};

#endif // DASH__TEST__GLOB_ASYNC_REF_TEST_H_
