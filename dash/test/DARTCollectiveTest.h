#ifndef DASH__TEST__DART_ONESIDED_TEST_H_
#define DASH__TEST__DART_ONESIDED_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for onesided operations provided by DART.
 */
class DARTCollectiveTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  DARTCollectiveTest() 
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: DARTCollectiveTest");
  }

  virtual ~DARTCollectiveTest() {
    LOG_MESSAGE("<<< Closing test suite: DARTCollectiveTest");
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

#endif // DASH__TEST__DART_ONESIDED_TEST_H_
