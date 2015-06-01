#ifndef DASH__TEST__PATTERN_TEST_H_
#define DASH__TEST__PATTERN_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Pattern
 */
class PatternTest : public ::testing::Test {
protected:
  int _num_elem;
  int _dash_size;

  PatternTest()
  : _num_elem(0), 
    _dash_size(0) {
  }

  virtual ~PatternTest() {
  }

  virtual void SetUp() {
    _num_elem  = 250;
    _dash_size = dash::size();
  }

  virtual void TearDown() {
  }
};

#endif // DASH__TEST__PATTERN_TEST_H_
