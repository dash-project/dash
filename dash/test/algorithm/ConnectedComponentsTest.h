#ifndef DASH__TEST__CONNECTED_COMPONENTS_TEST_H_
#define DASH__TEST__CONNECTED_COMPONENTS_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::List
 */
class ConnectedComponentsTest : public dash::test::TestBase {
protected:

  ConnectedComponentsTest() {
    LOG_MESSAGE(">>> Test suite: ConnectedComponentsTest");
  }

  virtual ~ConnectedComponentsTest() {
    LOG_MESSAGE("<<< Closing test suite: ConnectedComponentsTest");
  }
};

#endif // DASH__TEST__CONNECTED_COMPONENTS_TEST_H_
