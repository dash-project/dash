#ifndef DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
#define DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

#include "TestBase.h"

/**
 * Test fixture for class dash::GlobDynamicMem
 */
class GlobDynamicMemTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;

  GlobDynamicMemTest()
  : _dash_id(0),
    _dash_size(0)
  {
    LOG_MESSAGE(">>> Test suite: GlobDynamicMemTest");
  }

  virtual ~GlobDynamicMemTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobDynamicMemTest");
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
//  dash::barrier();
    LOG_MESSAGE("===> Running test case with %d units ...",
                _dash_size);
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
    LOG_MESSAGE("<=== Finished test case with %d units",
                _dash_size);
  }
};

#endif // DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
