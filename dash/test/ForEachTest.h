#ifndef DASH__TEST__FOR_EACH_TEST_H_
#define DASH__TEST__FOR_EACH_TEST_H_

#include "TestBase.h"

#include <dash/Array.h>

#include <vector>


/**
 * Test fixture for algorithm dash::for_each.
 */
class ForEachTest : public dash::test::TestBase {
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

public:
  void count_invoke(index_t index) {
    _invoked_indices.push_back(index);
  }
};

#endif // DASH__TEST__FOR_EACH_TEST_H_
