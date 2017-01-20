#ifndef DASH__TEST__MATRIX_TEST_H_
#define DASH__TEST__MATRIX_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Matrix
 */
class MatrixTest : public dash::test::TestBase {
protected:

  MatrixTest() {
    LOG_MESSAGE(">>> Test suite: MatrixTest");
  }

  virtual ~MatrixTest() {
    LOG_MESSAGE("<<< Closing test suite: MatrixTest");
  }
};

#endif // DASH__TEST__MATRIX_TEST_H_
