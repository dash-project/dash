#ifndef DASH__TEST__MATRIX_TEST_H_
#define DASH__TEST__MATRIX_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Matrix
 */
class MatrixTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  MatrixTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: MatrixTest");
  }

  virtual ~MatrixTest() {
    LOG_MESSAGE("<<< Closing test suite: MatrixTest");
  }
};

#endif // DASH__TEST__MATRIX_TEST_H_
