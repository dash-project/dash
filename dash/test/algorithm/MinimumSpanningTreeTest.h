#ifndef DASH__TEST__MINIMUM_SPANNING_TREE_TEST_H_
#define DASH__TEST__MINIMUM_SPANNING_TREE_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::List
 */
class MinimumSpanningTreeTest : public dash::test::TestBase {
protected:

  MinimumSpanningTreeTest() {
    LOG_MESSAGE(">>> Test suite: MinimumSpanningTreeTest");
  }

  virtual ~MinimumSpanningTreeTest() {
    LOG_MESSAGE("<<< Closing test suite: MinimumSpanningTreeTest");
  }
};

#endif // DASH__TEST__MINIMUM_SPANNING_TREE_TEST_H_
