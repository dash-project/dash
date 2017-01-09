#ifndef DASH__TEST__LIST_TEST_H_
#define DASH__TEST__LIST_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::List
 */
class ListTest : public dash::test::TestBase {
protected:

  ListTest() {
    LOG_MESSAGE(">>> Test suite: ListTest");
  }

  virtual ~ListTest() {
    LOG_MESSAGE("<<< Closing test suite: ListTest");
  }
};

#endif // DASH__TEST__LIST_TEST_H_
