#ifndef DASH__TEST__GENERATE_TEST_H_
#define DASH__TEST__GENERATE_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>

/**
 * Test fixture for class dash::generate
 */
class GenerateTest : public ::testing::Test {
protected:
  typedef double                  Element_t;
  typedef dash::Array<Element_t>  Array_t;
  typedef typename Array_t::pattern_type::index_type
    index_t;

  /// Using a prime to cause inconvenient strides
  size_t _num_elem      = 251;

  GenerateTest() {
  }

  virtual ~GenerateTest() {
  }

  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};
#endif // DASH__TEST__GENERATE_TEST_H_
