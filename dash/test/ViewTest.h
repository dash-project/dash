#ifndef DASH__TEST__VIEW_TEST_H_
#define DASH__TEST__VIEW_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include "TestBase.h"

/**
 * Test fixture for the DASH View concept
 */
class ViewTest : public dash::test::TestBase {
protected:

  ViewTest() {
    LOG_MESSAGE(">>> Test suite: ViewTest");
  }

  virtual ~ViewTest() {
    LOG_MESSAGE("<<< Closing test suite: ViewTest");
  }

};

#endif // DASH__TEST__VIEW_TEST_H_
