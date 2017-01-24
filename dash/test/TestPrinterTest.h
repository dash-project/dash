#ifndef DASH__TEST__FOR_EACH_TEST_H_
#define DASH__TEST__FOR_EACH_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for algorithm dash::for_each.
 */
class TestPrinterTest: public ::testing::Test {
protected:

  TestPrinterTest() {
  }

  virtual ~TestPrinterTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }

};

#endif // DASH__TEST__FOR_EACH_TEST_H_
