#ifndef DASH__TEST__FILL_TEST_H_
#define DASH__TEST__FILL_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::transform
 */
class FillTest : public ::testing::Test {
protected:

  FillTest() {
  }

  virtual ~FillTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};
#endif // DASH__TEST__FILL_TEST_H_
