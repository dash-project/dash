#ifndef DASH__TEST__TEAM_SPEC_TEST_H_
#define DASH__TEST__TEAM_SPEC_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::TeamSpec
 */
class TeamSpecTest : public ::testing::Test {
protected:

  TeamSpecTest() {
  }

  virtual ~TeamSpecTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__TEAM_SPEC_TEST_H_
