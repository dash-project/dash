#ifndef DASH__TEST__LOAD_BALANCE_PATTERN_TEST_H_
#define DASH__TEST__LOAD_BALANCE_PATTERN_TEST_H_

#include "TestBase.h"


/**
 * Test fixture for class dash::LoadBalancePattern
 */
class LoadBalancePatternTest : public dash::test::TestBase {
protected:

  LoadBalancePatternTest() {
    LOG_MESSAGE(">>> Test suite: LoadBalancePatternTest");
  }

  virtual ~LoadBalancePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: LoadBalancePatternTest");
  }
};

#endif // DASH__TEST__LOAD_BALANCE_PATTERN_TEST_H_
