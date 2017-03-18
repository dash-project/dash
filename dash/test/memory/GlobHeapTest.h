#ifndef DASH__TEST__GLOB_HEAP_TEST_H__INCLUDED
#define DASH__TEST__GLOB_HEAP_TEST_H__INCLUDED

#include "../TestBase.h"

/**
 * Test fixture for class dash::GlobHeap
 */
class GlobHeapTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;

  GlobHeapTest()
  : _dash_id(0),
    _dash_size(0)
  {
    LOG_MESSAGE(">>> Test suite: GlobHeapTest");
  }

  virtual ~GlobHeapTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobHeapTest");
  }
};

#endif // DASH__TEST__GLOB_HEAP_TEST_H__INCLUDED
