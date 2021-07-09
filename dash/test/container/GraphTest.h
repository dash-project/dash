#ifndef DASH__TEST__GRAPH_TEST_H_
#define DASH__TEST__GRAPH_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::List
 */
class GraphTest : public dash::test::TestBase {
protected:

  GraphTest() {
    LOG_MESSAGE(">>> Test suite: GraphTest");
  }

  virtual ~GraphTest() {
    LOG_MESSAGE("<<< Closing test suite: GraphTest");
  }
};

#endif // DASH__TEST__GRAPH_TEST_H_
