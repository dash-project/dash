#ifndef DASH__TEST__TEAM_TEST_H_
#define DASH__TEST__TEAM_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Team
 */
class TeamTest : public ::testing::Test {
protected:

  TeamTest() {
  }

  virtual ~TeamTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__TEAM_TEST_H_
