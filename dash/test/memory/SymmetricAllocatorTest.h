#ifndef DASH__TEST__ARRAY_TEST_H_
#define DASH__TEST__ARRAY_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::SymmetricAllocator
 */
class SymmetricAllocatorTest : public dash::test::TestBase {
protected:
  size_t _dash_id    = 0;
  size_t _dash_size  = 0;
  int    _num_elem   = 0;

  virtual void SetUp() {
    dash::test::TestBase::SetUp();
    _dash_id   = dash::myid();
    _dash_size = dash::size();
  }
};

#endif // DASH__TEST__ARRAY_TEST_H_
