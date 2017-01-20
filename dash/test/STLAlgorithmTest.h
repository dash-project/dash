#ifndef DASH__TEST__STL_ALGORITHM_TEST_H_
#define DASH__TEST__STL_ALGORITHM_TEST_H_

#include "TestBase.h"


/**
 * Test fixture for class dash::STLAlgorithm
 */
class STLAlgorithmTest : public dash::test::TestBase {
protected:

  STLAlgorithmTest()  {
    LOG_MESSAGE(">>> Test suite: STLAlgorithmTest");
  }

  virtual ~STLAlgorithmTest() {
    LOG_MESSAGE("<<< Closing test suite: STLAlgorithmTest");
  }
};

#endif // DASH__TEST__STL_ALGORITHM_TEST_H_
