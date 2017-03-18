#ifndef DASH__TEST__GLOBMEM_TEST_H_
#define DASH__TEST__GLOBMEM_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::GlobStaticHeap
 */
class GlobStaticHeapTest : public dash::test::TestBase {
protected:

  GlobStaticHeapTest() {
    LOG_MESSAGE(">>> Test suite: GlobStaticHeapTest");
  }

  virtual ~GlobStaticHeapTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobStaticHeapTest");
  }
};

#endif // DASH__TEST__GLOBMEM_TEST_H_
