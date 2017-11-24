#ifndef DASH__TEST__VARARGS_PATTERN_TEST_H_
#define DASH__TEST__VARARGS_PATTERN_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::Pattern
 */
class VarArgsPatternTest : public dash::test::TestBase {
protected:

  VarArgsPatternTest() {
    LOG_MESSAGE(">>> Test suite: VarArgsPatternTest");
  }

  virtual ~VarArgsPatternTest() {
    LOG_MESSAGE("<<< Closing test suite: VarArgsPatternTest");
  }
};

#endif // DASH__TEST__VARARGS_PATTERN_TEST_H_
