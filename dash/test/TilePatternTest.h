#ifndef DASH__TEST__TILE_PATTERN_TEST_H_
#define DASH__TEST__TILE_PATTERN_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::Pattern
 */
class TilePatternTest : public dash::test::TestBase {
protected:

  TilePatternTest() {
    LOG_MESSAGE(">>> Test suite: TilePatternTest");
  }

  virtual ~TilePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: TilePatternTest");
  }
};

#endif // DASH__TEST__TILE_PATTERN_TEST_H_
