#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "GlobDynamicMemTest.h"


TEST_F(GlobDynamicMemTest, Commit)
{
  typedef int value_t;

  if (dash::size() < 2) {
    LOG_MESSAGE("GlobDynamicMemTest.Commit requires at least two units");
    return;
  }

  LOG_MESSAGE("GlobDynamicMemTest.Commit: initializing GlobDynamicMem<T>");

  size_t initial_local_capacity  = 10;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  size_t initial_global_capacity = dash::size() * initial_local_capacity;

  ASSERT_EQ_U(initial_local_capacity,  gdmem.local_size());
  ASSERT_EQ_U(initial_local_capacity,
              gdmem.lend(dash::myid()) - gdmem.lbegin(dash::myid()));
  ASSERT_EQ_U(initial_global_capacity, gdmem.size());

  dash::barrier();

  if (dash::myid() == 0) {
    // Allocate another 512 elements in local memory space.
    // This is a local operation, the additionally allocated memory
    // space is only accessible by the local unit, however:
    LOG_MESSAGE("GlobDynamicMemTest.Commit: grow(5)");
    gdmem.grow(5);
  }
  if (dash::myid() == 1) {
    // Decrease capacity of local memory space by 128 units.
    // This is a local operation. New size of logical memory space is
    // effective for the local unit immediately but memory is not
    // physically freed yet and is still accessible by other units.
    LOG_MESSAGE("GlobDynamicMemTest.Commit: shrink(2)");
    gdmem.shrink(2);
  }

  dash::barrier();
  LOG_MESSAGE("GlobDynamicMemTest.Commit: global size: %d, local size: %d",
               gdmem.size(), gdmem.local_size());

  // Global memory space has not been updated yet, changes are only
  // visible locally.
  //
  // NOTE:
  // Local changes at units in same shared memory domain are visible
  // even when not committed yet.
  std::string my_host     = dash::util::Locality::Hostname(dash::myid());
  std::string unit_0_host = dash::util::Locality::Hostname(0);
  std::string unit_1_host = dash::util::Locality::Hostname(1);

  size_t expected_visible_size = initial_global_capacity;
  size_t expected_global_size  = initial_global_capacity;
  if (dash::myid() == 0) {
    expected_visible_size += 5;
    if (my_host == unit_1_host) {
      // same shared memory domain as unit 1, changes at unit 1 might already
      // be visible to this unit:
      ASSERT_TRUE_U(gdmem.size() == expected_visible_size ||
                    gdmem.size() == expected_visible_size - 2);
    } else {
      ASSERT_EQ_U(expected_visible_size, gdmem.size());
    }
  }
  if (dash::myid() == 1) {
    expected_visible_size -= 2;
    if (my_host == unit_0_host) {
      // same shared memory domain as unit 0, changes at unit 0 might already
      // be visible to this unit:
      ASSERT_TRUE_U(gdmem.size() == expected_visible_size ||
                    gdmem.size() == expected_visible_size + 5);
    } else {
      ASSERT_EQ_U(expected_visible_size, gdmem.size());
    }
  }

  dash::barrier();

  LOG_MESSAGE("GlobDynamicMemTest.Commit: testing local capacities after "
              "grow/shrink");

#if 0
  auto unit_0_local_size = gdmem.lend(0) - gdmem.lbegin(0);
  auto unit_1_local_size = gdmem.lend(1) - gdmem.lbegin(1);
  if (dash::myid() == 0) {
    ASSERT_EQ_U(unit_0_local_size, initial_local_capacity + 5);
    ASSERT_EQ_U(unit_0_local_size, gdmem.local_size());
  } else {
    ASSERT_EQ_U(unit_0_local_size, initial_local_capacity);
  }
  if (dash::myid() == 1) {
    ASSERT_EQ_U(unit_1_local_size, initial_local_capacity - 2);
    ASSERT_EQ_U(unit_1_local_size, gdmem.local_size());
  } else {
    ASSERT_EQ_U(unit_1_local_size, initial_local_capacity);
  }
#else
  auto local_size = gdmem.lend() - gdmem.lbegin();
  ASSERT_EQ_U(local_size, gdmem.local_size());
  if (dash::myid() == 0) {
    ASSERT_EQ_U(local_size, initial_local_capacity + 5);
  } else if (dash::myid() == 1) {
    ASSERT_EQ_U(local_size, initial_local_capacity - 2);
  } else {
    ASSERT_EQ_U(local_size, initial_local_capacity);
  }
#endif

  // Initialize values in local memory:
  LOG_MESSAGE("GlobDynamicMemTest.Commit: initialize local values");
  LOG_MESSAGE("GlobDynamicMemTest.Commit: local size: %d",
              gdmem.local_size());
  auto lbegin = gdmem.lbegin();
  for (size_t li = 0; li < gdmem.local_size(); ++li) {
    value_t value = 100 * (dash::myid() + 1) + li;
    DASH_LOG_TRACE("GlobDynamicMemTest.Commit", "local[", li, "] =", value);
    *(lbegin + li) = value;
  }

  dash::barrier();

  // Memory marked for deallocation is still accessible by other units:
#if 0
  if (dash::myid() != 1) {
    LOG_MESSAGE("GlobDynamicMemTest.Commit: requesting last local element "
                "from unit 1");
    dash::GlobPtr<value_t> unit_1_last(
                            gdmem.index_to_gptr(
                              1, initial_local_capacity-1));
    value_t actual;
    dash::get_value(&actual, unit_1_last);
    value_t expected = 100 * 2 + initial_local_capacity - 1;
    ASSERT_EQ_U(expected, actual);
  }
#endif

  LOG_MESSAGE("GlobDynamicMemTest.Commit: committing global memory");
  // Collectively commit changes of local memory allocation to global
  // memory space:
  // register newly allocated local memory and remove local memory marked
  // for deallocation.
  gdmem.commit();

  DASH_LOG_DEBUG("GlobDynamicMemTest.Commit",
                 "attached local buckets:", gdmem.buckets());

  // Changes are globally visible now:
  ASSERT_EQ_U(gdmem.size(), initial_global_capacity + 5 - 2);

#if 0
  unit_0_local_size = gdmem.lend(0) - gdmem.lbegin(0);
  unit_1_local_size = gdmem.lend(1) - gdmem.lbegin(1);
  ASSERT_EQ_U(unit_0_local_size, initial_local_capacity + 5);
  ASSERT_EQ_U(unit_1_local_size, initial_local_capacity - 2);
#endif
}
