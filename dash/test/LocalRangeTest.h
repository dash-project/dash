#ifndef DASH__TEST__LOCAL_RANGE_TEST_H_
#define DASH__TEST__LOCAL_RANGE_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for local range conversions like dash::local_range.
 */
class LocalRangeTest : public dash::test::TestBase {
protected:

  LocalRangeTest() {
    LOG_MESSAGE(">>> Test suite: LocalRangeTest");
  }

  virtual ~LocalRangeTest() {
    LOG_MESSAGE("<<< Closing test suite: LocalRangeTest");
  }
};

#endif // DASH__TEST__LOCAL_RANGE_TEST_H_
