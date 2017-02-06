#ifndef DASH__TEST__UTIL_TEST_H_
#define DASH__TEST__UTIL_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for the DASH helper types.
 */
class UtilTest : public dash::test::TestBase {
protected:

  UtilTest() {
    LOG_MESSAGE(">>> Test suite: UtilTest");
  }

  virtual ~UtilTest() {
    LOG_MESSAGE("<<< Closing test suite: UtilTest");
  }

};

#endif // DASH__TEST__UTIL_TEST_H_
