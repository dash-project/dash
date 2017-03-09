#ifndef DASH__TEST__TRANSFORM_TEST_H_
#define DASH__TEST__TRANSFORM_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::transform
 */
class TransformTest : public dash::test::TestBase {
protected:

  TransformTest()  {
    LOG_MESSAGE(">>> Test suite: TransformTest");
  }

  virtual ~TransformTest() {
    LOG_MESSAGE("<<< Closing test suite: TransformTest");
  }
};

#endif // DASH__TEST__TRANSFORM_TEST_H_
