#ifndef DASH__TEST__GLOBMEM_TEST_H_
#define DASH__TEST__GLOBMEM_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::GlobStaticMem
 */
class GlobStaticMemTest : public dash::test::TestBase {
protected:

  GlobStaticMemTest() {
    LOG_MESSAGE(">>> Test suite: GlobStaticMemTest");
  }

  virtual ~GlobStaticMemTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobStaticMemTest");
  }
};

#endif // DASH__TEST__GLOBMEM_TEST_H_
