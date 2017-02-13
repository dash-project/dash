#ifndef DASH__TEST__UTIL_TEST_H_
#define DASH__TEST__UTIL_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for the DASH helper types.
 */
class UniversalMemberTest : public dash::test::TestBase {
protected:

  UniversalMemberTest() {
    LOG_MESSAGE(">>> Test suite: UniversalMemberTest");
  }

  virtual ~UniversalMemberTest() {
    LOG_MESSAGE("<<< Closing test suite: UniversalMemberTest");
  }

};

#endif // DASH__TEST__UTIL_TEST_H_
