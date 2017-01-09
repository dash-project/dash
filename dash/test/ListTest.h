#ifndef DASH__TEST__LIST_TEST_H_
#define DASH__TEST__LIST_TEST_H_

#include "TestBase.h"

/**
 * Test fixture for class dash::List
 */
class ListTest : public dash::test::TestBase {
protected:
  const size_t _dash_id;
  const size_t _dash_size;
  const int    _num_elem;

  ListTest()
  : _dash_id(dash::myid().id),
    _dash_size(dash::size()),
    _num_elem(100) {
    LOG_MESSAGE(">>> Test suite: ListTest");
  }

  virtual ~ListTest() {
    LOG_MESSAGE("<<< Closing test suite: ListTest");
  }
};

#endif // DASH__TEST__LIST_TEST_H_
