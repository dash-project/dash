#ifndef DASH__TEST__STL_ALGO_TEST_H_
#define DASH__TEST__STL_ALGO_TEST_H_

#include "TestBase.h"

/**
 * Tests compability of dash iterators and STL algorithms 
 */
class StlAlgoTest : public dash::test::TestBase {
protected:

  StlAlgoTest() {
    LOG_MESSAGE(">>> Test suite: StlAlgoTest");
  }

  virtual ~StlAlgoTest() {
    LOG_MESSAGE("<<< Closing test suite: StlAlgoTest");
  }
};

#endif // DASH__TEST__STL_ALGO_TEST_H_ 
