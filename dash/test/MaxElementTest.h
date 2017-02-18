#ifndef DASH__TEST__MAX_ELEMENT_TEST_H_
#define DASH__TEST__MAX_ELEMENT_TEST_H_

#include "TestBase.h"

#include <dash/Array.h>


/**
 * Test fixture for algorithm dash::max_element.
 */
class MaxElementTest : public dash::test::TestBase {
protected:
  typedef long                   Element_t;
  typedef dash::Array<Element_t> Array_t;
  typedef typename Array_t::pattern_type::index_type
    index_t;

  /// Using a prime to cause inconvenient strides
  const size_t _num_elem = 251;

  MaxElementTest() {
  }

  virtual ~MaxElementTest() {
  }
};

#endif // DASH__TEST__MAX_ELEMENT_TEST_H_
