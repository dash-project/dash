#ifndef DASH__TEST__CARTESIAN_TEST_H_
#define DASH__TEST__CARTESIAN_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Cartesian
 */
class CartesianTest : public ::testing::Test {
protected:

  CartesianTest() {
  }

  virtual ~CartesianTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__CARTESIAN_TEST_H_
