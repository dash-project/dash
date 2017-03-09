#ifndef DASH__TEST__COPY_TEST_H_
#define DASH__TEST__COPY_TEST_H_

#include <gtest/gtest.h>

#include "TestBase.h"


/**
 * Test fixture for \c dash::copy.
 */
class CopyTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  CopyTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: CopyTest");
  }

  virtual ~CopyTest() {
    LOG_MESSAGE("<<< Closing test suite: CopyTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__COPY_TEST_H_
