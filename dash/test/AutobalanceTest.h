#ifndef DASH__TEST__AUTOBALANCE_TEST_H_
#define DASH__TEST__AUTOBALANCE_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Autobalance
 */
class AutobalanceTest : public ::testing::Test {
protected:

  AutobalanceTest() {
  }

  virtual ~AutobalanceTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__AUTOBALANCE_TEST_H_
