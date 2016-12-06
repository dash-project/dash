#ifndef DASH__TEST__CONFIG_TEST_H_
#define DASH__TEST__CONFIG_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Config
 */
class ConfigTest : public ::testing::Test {
protected:

  ConfigTest() {
  }

  virtual ~ConfigTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__CONFIG_TEST_H_
