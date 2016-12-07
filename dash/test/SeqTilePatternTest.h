#ifndef DASH__TEST__SEQ_TILE_PATTERN_TEST_H_
#define DASH__TEST__SEQ_TILE_PATTERN_TEST_H_

#include "TestBase.h"
#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Pattern
 */
class SeqTilePatternTest : public ::testing::Test {
protected:
  int _num_elem;
  int _dash_size;

  SeqTilePatternTest()
  : _num_elem(0),
    _dash_size(0) {
    LOG_MESSAGE(">>> Test suite: SeqTilePatternTest");
  }

  virtual ~SeqTilePatternTest() {
    LOG_MESSAGE("<<< Closing test suite: SeqTilePatternTest");
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
    _num_elem  = 250;
    _dash_size = dash::size();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
    dash::finalize();
  }
};

#endif // DASH__TEST__TILE_PATTERN_TEST_H_
