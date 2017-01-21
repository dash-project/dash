#ifndef DASH__TEST__MULTI_VIEW_TEST_H_
#define DASH__TEST__MULTI_VIEW_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for the DASH View concept
 */
class MultiViewTest : public dash::test::TestBase {
protected:

  MultiViewTest() {
    LOG_MESSAGE(">>> Test suite: MultiViewTest");
  }

  virtual ~MultiViewTest() {
    LOG_MESSAGE("<<< Closing test suite: MultiViewTest");
  }

};

#endif // DASH__TEST__MULTI_VIEW_TEST_H_
