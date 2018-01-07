#ifndef DASH__TEST__SORT_TEST_H_
#define DASH__TEST__SORT_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::sort
 */
class SortTest : public dash::test::TestBase {
protected:
  size_t const num_local_elem = 100;

  SortTest()
  {
  }

  virtual ~SortTest()
  {
  }
};
#endif  // DASH__TEST__SORT_TEST_H_
