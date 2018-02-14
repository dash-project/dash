#ifndef DASH__TEST__LOCALALLOC_TEST_H_
#define DASH__TEST__LOCALALLOC_TEST_H_

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
      dash::global_allocation_policy::local>;

  template <typename T>
  using local_allocator_t =
      dash::SymmetricAllocator<T, dash::global_allocation_policy::local>;

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
