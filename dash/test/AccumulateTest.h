#ifndef DASH__TEST__ACCUMULATE_TEST_H_
#define DASH__TEST__ACCUMULATE_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include "TestBase.h"

/**
 * Test fixture for class dash::accumulate
 */
class AccumulateTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  AccumulateTest() 
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: AccumulateTest");
  }

  virtual ~AccumulateTest() {
    LOG_MESSAGE("<<< Closing test suite: AccumulateTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }

  virtual void TearDown() {
    dash::test::TestBase::TearDown();
  }
};

#endif // DASH__TEST__ACCUMULATE_TEST_H_
