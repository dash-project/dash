#ifndef DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
#define DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::GlobDynamicMem
 */
class GlobDynamicMemTest : public dash::test::TestBase {
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
};

#endif // DASH__TEST__GLOB_DYNAMIC_MEM_TEST_H_
