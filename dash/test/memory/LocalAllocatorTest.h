#ifndef DASH__TEST__LOCALALLOC_TEST_H_
#define DASH__TEST__LOCALALLOC_TEST_H_

#include <dash/Allocator.h>
#include "../TestBase.h"

/**
 * Test fixture for class dash::LocalAllocator
 */
class LocalAllocatorTest : public dash::test::TestBase {
protected:
  template <typename T>
  using glob_mem_t = dash::GlobStaticMem<
      T,
      dash::HostSpace,
      dash::global_allocation_policy::non_collective>;

protected:
  LocalAllocatorTest()
  {
    LOG_MESSAGE(">>> Test suite: LocalAllocatorTest");
  }

  virtual ~LocalAllocatorTest()
  {
    LOG_MESSAGE("<<< Closing test suite: SymmetricAllocatorTest");
  }
};

#endif  // DASH__TEST__LOCALALLOC_TEST_H_
