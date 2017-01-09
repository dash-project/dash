#ifndef DASH__TEST__GLOB_ASYNC_REF_TEST_H_
#define DASH__TEST__GLOB_ASYNC_REF_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for non-blocking operations using \c dash::GlobAsyncRef.
 */
class GlobAsyncRefTest : public dash::test::TestBase {
protected:
  dash::global_unit_t _dash_id;
  size_t              _dash_size;

  GlobAsyncRefTest()
  : _dash_id(0),
    _dash_size(dash::size()) {
    LOG_MESSAGE(">>> Test suite: GlobAsyncRefTest");
  }

  virtual ~GlobAsyncRefTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobAsyncRefTest");
  }
};

#endif // DASH__TEST__GLOB_ASYNC_REF_TEST_H_
