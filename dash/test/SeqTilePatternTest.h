#ifndef DASH__TEST__SEQ_TILE_PATTERN_TEST_H_
#define DASH__TEST__SEQ_TILE_PATTERN_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Pattern
 */
class SeqTilePatternTest : public dash::test::TestBase {
protected:

  SeqTilePatternTest() {
    LOG_MESSAGE(">>> Test suite: SeqTilePatternTest");
  }

  virtual ~SeqTilePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: SeqTilePatternTest");
  }

};

#endif // DASH__TEST__TILE_PATTERN_TEST_H_
