#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "GlobDynamicMemTest.h"


TEST_F(GlobDynamicMemTest, LocalVisibility)
{
  typedef int value_t;

  if (dash::size() < 2) {
    LOG_MESSAGE(
      "GlobDynamicMemTest.LocalVisibility requires at least two units");
    return;
  }

  LOG_MESSAGE("initializing GlobDynamicMem<T>");

  size_t initial_local_capacity = 10;
  size_t local_buffer_capacity  = 10;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity,
                                      local_buffer_capacity);
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  initial_local_capacity  += local_buffer_capacity;
  initial_global_capacity += local_buffer_capacity;

  EXPECT_EQ_U(initial_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(initial_local_capacity,
              gdmem.lend(dash::myid()) - gdmem.lbegin(dash::myid()));
  EXPECT_EQ_U(initial_global_capacity, gdmem.size());

  dash::barrier();

  int unit_0_lsize_diff =  5;
  int unit_1_lsize_diff = -2;

  if (dash::myid() == 0) {
    LOG_MESSAGE("grow(5)");
    gdmem.grow(unit_0_lsize_diff);
  }
  if (dash::myid() == 1) {
    LOG_MESSAGE("shrink(2)");
    gdmem.shrink(-unit_1_lsize_diff);
  }

  dash::barrier();
  LOG_MESSAGE("global size: %d, local size: %d",
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
    expected_visible_size += unit_0_lsize_diff;
    if (my_host == unit_1_host) {
      LOG_MESSAGE("expected visible size: %d (locally) or %d (shmem)",
                  expected_visible_size,
                  expected_visible_size + unit_1_lsize_diff);
      // same shared memory domain as unit 1, changes at unit 1 might already
      // be visible to this unit:
      EXPECT_TRUE_U(
        gdmem.size() == expected_visible_size ||
        gdmem.size() == expected_visible_size + unit_1_lsize_diff);
    } else {
      EXPECT_EQ_U(expected_visible_size, gdmem.size());
    }
  }
  if (dash::myid() == 1) {
    expected_visible_size += unit_1_lsize_diff;
    if (my_host == unit_0_host) {
      LOG_MESSAGE("expected visible size: %d (locally) or %d (shmem)",
                  expected_visible_size,
                  expected_visible_size + unit_0_lsize_diff);
      // same shared memory domain as unit 0, changes at unit 0 might already
      // be visible to this unit:
      EXPECT_TRUE_U(
        gdmem.size() == expected_visible_size ||
        gdmem.size() == expected_visible_size + unit_0_lsize_diff);
    } else {
      EXPECT_EQ_U(expected_visible_size, gdmem.size());
    }
  }

  dash::barrier();

  LOG_MESSAGE("testing local capacities after grow/shrink");

  auto local_size = gdmem.lend() - gdmem.lbegin();
  EXPECT_EQ_U(local_size, gdmem.local_size());
  if (dash::myid() == 0) {
    EXPECT_EQ_U(local_size, initial_local_capacity + unit_0_lsize_diff);
  } else if (dash::myid() == 1) {
    EXPECT_EQ_U(local_size, initial_local_capacity + unit_1_lsize_diff);
  } else {
    EXPECT_EQ_U(local_size, initial_local_capacity);
  }

  // Initialize values in local memory:
  LOG_MESSAGE("initialize local values");
  LOG_MESSAGE("local size: %d",
              gdmem.local_size());
  auto lbegin = gdmem.lbegin();
  for (size_t li = 0; li < gdmem.local_size(); ++li) {
    value_t value = 100 * (dash::myid() + 1) + li;
    DASH_LOG_TRACE("GlobDynamicMemTest.Commit", "local[", li, "] =", value);
    *(lbegin + li) = value;
  }

  dash::barrier();

  // Memory marked for deallocation is still accessible by other units:

  LOG_MESSAGE("committing global memory");
  // Collectively commit changes of local memory allocation to global
  // memory space:
  // register newly allocated local memory and remove local memory marked
  // for deallocation.
  gdmem.commit();

  DASH_LOG_DEBUG("GlobDynamicMemTest.Commit",
                 "attached local buckets:", gdmem.buckets());

  // Changes are globally visible now:
  EXPECT_EQ_U(
    gdmem.size(),
    initial_global_capacity + unit_0_lsize_diff + unit_1_lsize_diff);
}

TEST_F(GlobDynamicMemTest, LocalMultipleResize)
{
  typedef int value_t;

  if (dash::size() < 2) {
    LOG_MESSAGE(
      "GlobDynamicMemTest.LocalMultipleResize requires at least two units");
    return;
  }

  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  EXPECT_EQ_U(initial_local_capacity, gdmem.local_size());
  EXPECT_EQ_U(initial_local_capacity,
              gdmem.lend(dash::myid()) - gdmem.lbegin(dash::myid()));
  EXPECT_EQ_U(initial_global_capacity, gdmem.size());

  dash::barrier();

  int unit_0_lsize_diff =  5;
  int unit_1_lsize_diff = -2;

  if (dash::myid() == 0) {
    gdmem.grow(3);
    gdmem.shrink(2);
    gdmem.grow(5);
    gdmem.shrink(1);
  }
  if (dash::myid() == 1) {
    gdmem.shrink(2);
    gdmem.grow(5);
    gdmem.shrink(2);
    gdmem.shrink(3);
  }

  gdmem.commit();

  DASH_LOG_DEBUG("GlobDynamicMemTest.LocalMultipleResize",
                 "attached local buckets:", gdmem.buckets());
}

TEST_F(GlobDynamicMemTest, RemoteAccess)
{
  typedef int value_t;

  if (dash::size() < 3) {
    LOG_MESSAGE(
      "GlobDynamicMemTest.RemoteAccess requires at least three units");
    return;
  }

  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  EXPECT_EQ_U(initial_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(initial_local_capacity,
              gdmem.lend(dash::myid()) - gdmem.lbegin(dash::myid()));
  EXPECT_EQ_U(initial_global_capacity, gdmem.size());

  int unit_0_lsize_diff =  5;
  int unit_1_lsize_diff = -2;

  if (dash::myid() == 0) {
    gdmem.resize(initial_global_capacity + unit_0_lsize_diff);
  }
  if (dash::myid() == 1) {
    gdmem.resize(initial_global_capacity + unit_1_lsize_diff);
  }

  dash::barrier();

  if (dash::myid() > 1) {
    auto unit_0_local_size = gdmem.lend(0) - gdmem.lbegin(0);
    auto unit_1_local_size = gdmem.lend(1) - gdmem.lbegin(1);
    if (dash::myid() == 0) {
      EXPECT_EQ_U(unit_0_local_size, initial_local_capacity + 5);
      EXPECT_EQ_U(unit_0_local_size, gdmem.local_size());
    } else {
      EXPECT_EQ_U(unit_0_local_size, initial_local_capacity);
    }
    if (dash::myid() == 1) {
      EXPECT_EQ_U(unit_1_local_size, initial_local_capacity - 2);
      EXPECT_EQ_U(unit_1_local_size, gdmem.local_size());
    } else {
      EXPECT_EQ_U(unit_0_local_size, initial_local_capacity);
      EXPECT_EQ_U(unit_1_local_size, initial_local_capacity);
    }
  }

  if (dash::myid() != 1) {
    LOG_MESSAGE("requesting last local element from unit 1");
    dash::GlobPtr<value_t> unit_1_last(
                            gdmem.index_to_gptr(
                              1, initial_local_capacity-1));
    value_t actual;
    dash::get_value(&actual, unit_1_last);
    value_t expected = 100 * 2 + initial_local_capacity - 1;
    EXPECT_EQ_U(expected, actual);
  }
}
