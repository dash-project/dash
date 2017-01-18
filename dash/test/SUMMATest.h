#ifndef DASH__TEST__SUMMA_TEST_H_
#define DASH__TEST__SUMMA_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for algorithm \c dash::summa.
 */
class SUMMATest : public dash::test::TestBase {
protected:

  SUMMATest() {
    LOG_MESSAGE(">>> Test suite: SUMMATest");
  }

  virtual ~SUMMATest() {
    LOG_MESSAGE("<<< Closing test suite: SUMMATest");
  }
};

#endif // DASH__TEST__SUMMA_TEST_H_
