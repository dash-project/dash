#ifndef DASH__TEST__BLOCK_PATTERN_TEST_H_
#define DASH__TEST__BLOCK_PATTERN_TEST_H_

#include "TestBase.h"


/**
 * Test fixture for class dash::Pattern
 */
class BlockPatternTest : public dash::test::TestBase {
protected:
  int _dash_size;
  int _num_elem;

  BlockPatternTest()
  : _dash_size(0),
    _num_elem(23) {
    LOG_MESSAGE(">>> Test suite: BlockPatternTest");
  }

  virtual ~BlockPatternTest() {
    LOG_MESSAGE("<<< Closing test suite: BlockPatternTest");
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_size = dash::size();
    _num_elem  = 23;
  }
};

#endif // DASH__TEST__BLOCK_PATTERN_TEST_H_
