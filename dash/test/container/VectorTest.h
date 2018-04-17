#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include <gtest/gtest.h>

#include "../TestBase.h"

/**
 * Test fixture for class dash::Array
 */
class VectorTest : public dash::test::TestBase {
protected:
  size_t _dash_id;
  size_t _dash_size;
  int _num_elem;

  VectorTest()
  : _dash_id(0),
    _dash_size(0),
    _num_elem(0) {
  }

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    _num_elem  = 100;
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
