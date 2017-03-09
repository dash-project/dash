#ifndef DASH__TEST__RANGE_TEST_H_
#define DASH__TEST__RANGE_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for the DASH Range concept
 */
class RangeTest : public dash::test::TestBase {
protected:

  RangeTest() {
    LOG_MESSAGE(">>> Test suite: RangeTest");
  }

  virtual ~RangeTest() {
    LOG_MESSAGE("<<< Closing test suite: RangeTest");
  }

};

#endif // DASH__TEST__RANGE_TEST_H_
