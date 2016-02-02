#ifndef DASH__TEST__FILL_TEST_H_
#define DASH__TEST__FILL_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::transform
 */
class FillTest : public ::testing::Test {
protected:
  typedef double                  Element_t;
  typedef dash::Array<Element_t>  Array_t;

  /// Using a prime to cause inconvenient strides
  size_t _num_elem      = 251;

  FillTest() {
  }

  virtual ~FillTest() {
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
#endif // DASH__TEST__FILL_TEST_H_
