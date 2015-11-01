#ifndef DASH__TEST__MAKE_PATTERN_TEST_H_
#define DASH__TEST__MAKE_PATTERN_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for function dash::make_pattern.
 */
class MakePatternTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  MakePatternTest() 
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: MakePatternTest");
  }

  virtual ~MakePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: MakePatternTest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
  }
};

#endif // DASH__TEST__MAKE_PATTERN_TEST_H_
