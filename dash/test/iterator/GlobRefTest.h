#ifndef DASH__TEST__GLOB_REF_TEST_H_
#define DASH__TEST__GLOB_REF_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for non-blocking operations using \c dash::GlobAsyncRef.
 */
class GlobRefTest : public dash::test::TestBase {
protected:

  GlobRefTest() {
    LOG_MESSAGE(">>> Test suite: GlobAsyncRefTest");
  }

  virtual ~GlobRefTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobAsyncRefTest");
  }
};

#endif // DASH__TEST__GLOB_REF_TEST_H_
