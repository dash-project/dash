#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::GlobMem
 */
class GlobMemTest : public dash::test::TestBase {
protected:

  GlobMemTest() {
    LOG_MESSAGE(">>> Test suite: GlobMemTest");
  }

  virtual ~GlobMemTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobMemTest");
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
