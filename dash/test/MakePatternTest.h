#ifndef DASH__TEST__MAKE_PATTERN_TEST_H_
#define DASH__TEST__MAKE_PATTERN_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for function dash::make_pattern.
 */
class MakePatternTest : public dash::test::TestBase {
protected:

  MakePatternTest()  {
    LOG_MESSAGE(">>> Test suite: MakePatternTest");
  }

  virtual ~MakePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: MakePatternTest");
  }
};

#endif // DASH__TEST__MAKE_PATTERN_TEST_H_
