#ifndef DASH__TEST__LOCAL_RANGE_TEST_H_
#define DASH__TEST__LOCAL_RANGE_TEST_H_

#include "TestBase.h"

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for local range conversions like dash::local_range.
 */
class LocalRangeTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  LocalRangeTest()
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: LocalRangeTest");
  }

  virtual ~LocalRangeTest() {
    LOG_MESSAGE("<<< Closing test suite: LocalRangeTest");
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    dash::barrier();
    LOG_MESSAGE("===> Running test case with %ld units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %ld units",
                _dash_size);
    dash::finalize();
  }
};

#endif // DASH__TEST__LOCAL_RANGE_TEST_H_
