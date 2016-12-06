#ifndef DASH__TEST__FOR_EACH_TEST_H_
#define DASH__TEST__FOR_EACH_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include <vector>

/**
 * Test fixture for algorithm dash::for_each.
 */
class ForEachTest : public ::testing::Test {
protected:
  typedef double                  Element_t;
  typedef dash::Array<Element_t>  Array_t;
  typedef typename Array_t::pattern_type::index_type
    index_t;

  /// Using a prime to cause inconvenient strides
  size_t _num_elem      = 251;
  /// Stores indices passed to count_invoke
  std::vector<index_t> _invoked_indices;

  ForEachTest() {
  }

  virtual ~ForEachTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }

public:
  void count_invoke(index_t index) {
    _invoked_indices.push_back(index);
  }
};

#endif // DASH__TEST__FOR_EACH_TEST_H_
