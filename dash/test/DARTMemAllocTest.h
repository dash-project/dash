#ifndef DASH__TEST__DART_ONESIDED_TEST_H_
#define DASH__TEST__DART_ONESIDED_TEST_H_

#include "TestBase.h"


/**
 * Test fixture for onesided operations provided by DART.
 */
class DARTMemAllocTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  DARTMemAllocTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: DARTOnesidedTest");
  }

  virtual ~DARTMemAllocTest() {
    LOG_MESSAGE("<<< Closing test suite: DARTOnesidedTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__DART_ONESIDED_TEST_H_
