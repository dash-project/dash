#ifndef DASH__TEST__UNIT_ID_TEST_H__INCLUDED
#define DASH__TEST__UNIT_ID_TEST_H__INCLUDED

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class DASH unit id types.
 */
class UnitIdTest : public ::testing::Test {
protected:

  UnitIdTest() {
  }

  virtual ~UnitIdTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__UNIT_ID_TEST_H__INCLUDED
