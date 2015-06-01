#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::Array
 */
class ArrayTest : public ::testing::Test {
protected:
  size_t _dash_id;
  size_t _dash_size;
  int _num_elem;

  ArrayTest() 
  : _dash_id(0),
    _dash_size(0),
    _num_elem(0) {
  }

  virtual ~ArrayTest() {
  }

  virtual void SetUp() {
    _dash_id   = dash::myid();
    _dash_size = dash::size();
    _num_elem  = 10;
  }

  virtual void TearDown() {
    dash::Team::All().barrier();
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
