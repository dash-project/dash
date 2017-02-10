#ifndef DASH__TEST__NVIEW_TEST_H_
#define DASH__TEST__NVIEW_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for the DASH View concept
 */
class NViewTest : public dash::test::TestBase {
protected:

  NViewTest() {
    LOG_MESSAGE(">>> Test suite: NViewTest");
  }

  virtual ~NViewTest() {
    LOG_MESSAGE("<<< Closing test suite: NViewTest");
  }

};

#endif // DASH__TEST__NVIEW_TEST_H_
