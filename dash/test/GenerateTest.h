#ifndef DASH__TEST__GENERATE_TEST_H_
#define DASH__TEST__GENERATE_TEST_H_

#include "TestBase.h"

#include <dash/Array.h>


/**
 * Test fixture for class dash::generate
 */
class GenerateTest : public dash::test::TestBase
{
protected:
  typedef double                         Element_t;
  typedef dash::Array<Element_t>         Array_t;
  typedef typename Array_t::pattern_type Pattern_t;
  typedef typename Pattern_t::index_type index_t;

  /// Using a prime to cause inconvenient strides
  size_t _num_elem = 251;

  GenerateTest() {
  }

  virtual ~GenerateTest() {
  }
};
#endif // DASH__TEST__GENERATE_TEST_H_
