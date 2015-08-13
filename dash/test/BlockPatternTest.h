#ifndef DASH__TEST__BLOCK_PATTERN_TEST_H_
#define DASH__TEST__BLOCK_PATTERN_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Pattern
 */
class BlockPatternTest : public ::testing::Test {
protected:
  int _num_elem;
  int _dash_size;

  BlockPatternTest()
  : _num_elem(0), 
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: BlockPatternTest");
  }

  virtual ~BlockPatternTest() {
    LOG_MESSAGE("<<< Closing test suite: BlockPatternTest");
  }

  virtual void SetUp() {
    _num_elem  = 250;
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
  }
};

#endif // DASH__TEST__BLOCK_PATTERN_TEST_H_
