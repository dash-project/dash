#ifndef DASH__TEST__CONSTEXPR_TEST_H__INCLUDED
#define DASH__TEST__CONSTEXPR_TEST_H__INCLUDED

#include "TestBase.h"

/**
 * Test fixture for the DASH Constexpr concept
 */
class ConstexprTest : public dash::test::TestBase {
protected:

  ConstexprTest() {
    LOG_MESSAGE(">>> Test suite: ConstexprTest");
  }

  virtual ~ConstexprTest() {
    LOG_MESSAGE("<<< Closing test suite: ConstexprTest");
  }

};

#endif // DASH__TEST__CONSTEXPR_TEST_H__INCLUDED
