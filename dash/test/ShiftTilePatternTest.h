#ifndef DASH__TEST__SHIFT_TILE_PATTERN_TEST_H_
#define DASH__TEST__SHIFT_TILE_PATTERN_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::ShiftTilePattern
 */
class ShiftTilePatternTest : public dash::test::TestBase {
protected:

  ShiftTilePatternTest() {
    LOG_MESSAGE(">>> Test suite: ShiftTilePatternTest");
  }

  virtual ~ShiftTilePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: ShiftTilePatternTest");
  }
};

#endif // DASH__TEST__TILE_PATTERN_TEST_H_
