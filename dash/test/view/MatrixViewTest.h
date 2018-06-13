#ifndef DASH__TEST__MATRIX_VIEW_TEST_H__INCLUDED
#define DASH__TEST__MATRIX_VIEW_TEST_H__INCLUDED

#include "ViewTestBase.h"

/**
 * Test fixture for the DASH View concept
 */
class MatrixViewTest : public dash::test::ViewTestBase {
protected:

  MatrixViewTest() {
    LOG_MESSAGE(">>> Test suite: MatrixViewTest");
  }

  virtual ~MatrixViewTest() {
    LOG_MESSAGE("<<< Closing test suite: MatrixViewTest");
  }

};

#endif // DASH__TEST__MATRIX_VIEW_TEST_H__INCLUDED
