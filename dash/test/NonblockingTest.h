#ifndef DASH__TEST__NONBLOCKING_TEST_H_
#define DASH__TEST__NONBLOCKING_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for non-blocking operations.
 */
class NonblockingTest : public ::testing::Test {
protected:
  dart_unit_t _dash_id;
  size_t      _dash_size;

  NonblockingTest() 
  : _dash_id(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: NonblockingTest");
  }

  virtual ~NonblockingTest() {
    LOG_MESSAGE("<<< Closing test suite: NonblockingTest");
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

#endif // DASH__TEST__NONBLOCKING_TEST_H_
