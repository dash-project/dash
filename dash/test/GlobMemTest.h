#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::GlobMem
 */
class GlobMemTest : public dash::test::TestBase {
protected:
  const size_t _dash_id;
  const size_t _dash_size;
  int          _num_elem;

  GlobMemTest()
  : _dash_id(dash::myid().id),
    _dash_size(dash::size()),
    _num_elem(100) {
    LOG_MESSAGE(">>> Test suite: GlobMemTest");
  }

  virtual ~GlobMemTest() {
    LOG_MESSAGE("<<< Closing test suite: GlobMemTest");
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
