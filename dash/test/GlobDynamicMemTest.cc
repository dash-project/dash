#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "GlobDynamicMemTest.h"

TEST_F(GlobDynamicMemTest, BalancedAlloc)
{
  typedef int value_t;

  if (dash::size() < 2) {
    SKIP_TEST_MSG("Test case requires at least two units");
  }

  LOG_MESSAGE("initializing GlobDynamicMem<T>");

  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  LOG_MESSAGE("initial global capacity: %ld, initial local capacity: %ld",
              initial_global_capacity, initial_local_capacity);

  EXPECT_EQ_U(initial_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(initial_local_capacity,  gdmem.lend() -
                                       gdmem.lbegin());
  EXPECT_EQ_U(initial_global_capacity, gdmem.size());

  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "initial local:",
                 gdmem.local_size());
  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "initial global:",
                 gdmem.size());
  // Wait for validation of initial capacity at all units:
  dash::barrier();

  size_t bucket_1_size = 5;
  size_t bucket_2_size = 7;

  gdmem.grow(3);
  gdmem.grow(bucket_1_size);
  gdmem.grow(bucket_2_size);
  gdmem.shrink(3);

  size_t precommit_local_capacity  = initial_local_capacity +
                                     bucket_1_size + bucket_2_size;
  size_t precommit_global_capacity = initial_global_capacity +
                                     bucket_1_size + bucket_2_size;
  EXPECT_EQ_U(precommit_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(precommit_local_capacity,  gdmem.lend() -
                                         gdmem.lbegin());
  EXPECT_EQ_U(precommit_global_capacity, gdmem.size());

  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "pre-commit local:",
                 gdmem.local_size());
  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "pre-commit global:",
                 gdmem.size());
  // Wait for validation of changes of local capacity at all units:
  dash::barrier();

  gdmem.commit();

  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "post-commit local:",
                 gdmem.local_size());
  DASH_LOG_TRACE("GlobDynamicMemTest.BalancedAlloc", "post-commit global:",
                 gdmem.size());
  size_t postcommit_local_capacity  = precommit_local_capacity;
  size_t postcommit_global_capacity = dash::size() *
                                      postcommit_local_capacity;
  EXPECT_EQ_U(postcommit_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(postcommit_local_capacity,  gdmem.lend() -
                                          gdmem.lbegin());
  EXPECT_EQ_U(postcommit_global_capacity, gdmem.size());
}

TEST_F(GlobDynamicMemTest, UnbalancedRealloc)
{
  typedef int value_t;

  if (dash::size() < 2) {
    SKIP_TEST_MSG("Test case requires at least two units");
  }

  LOG_MESSAGE("initializing GlobDynamicMem<T>");

  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  LOG_MESSAGE("initial global capacity: %ld, initial local capacity: %ld",
              initial_global_capacity, initial_local_capacity);

  EXPECT_EQ_U(initial_local_capacity,  gdmem.local_size());
  EXPECT_EQ_U(initial_local_capacity,  gdmem.lend() -
                                       gdmem.lbegin());
  EXPECT_EQ_U(initial_global_capacity, gdmem.size());

  dash::barrier();

  // Total changes of local capacity:
  int unit_0_lsize_diff = 120;
  int unit_1_lsize_diff =   6;
  int unit_x_lsize_diff =   5;
  int gsize_diff        = unit_0_lsize_diff +
                          unit_1_lsize_diff +
                          (dash::size() - 2) * unit_x_lsize_diff;

  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "begin local reallocation");
  // Extend local size, changes should be locally visible immediately:
  if (dash::myid() == 0) {
    gdmem.grow(unit_0_lsize_diff);
    EXPECT_EQ_U(initial_local_capacity + unit_0_lsize_diff,
                gdmem.local_size());
  }
  else if (dash::myid() == 1) {
    gdmem.grow(unit_1_lsize_diff);
    EXPECT_EQ_U(initial_local_capacity + unit_1_lsize_diff,
                gdmem.local_size());
  } else {
    gdmem.grow(unit_x_lsize_diff);
    EXPECT_EQ_U(initial_local_capacity + unit_x_lsize_diff,
                gdmem.local_size());
  }

  dash::barrier();
  LOG_MESSAGE("before commit: global size: %d, local size: %d",
               gdmem.size(), gdmem.local_size());

  gdmem.commit();

  LOG_MESSAGE("after commit: global size: %d, local size: %d",
               gdmem.size(), gdmem.local_size());

  // Global size should be updated after commit:
  EXPECT_EQ_U(initial_global_capacity + gsize_diff, gdmem.size());

  // Local sizes should be unchanged after commit:
  if (dash::myid() == 0) {
    EXPECT_EQ_U(initial_local_capacity + unit_0_lsize_diff,
                gdmem.local_size());
  }
  else if (dash::myid() == 1) {
    EXPECT_EQ_U(initial_local_capacity + unit_1_lsize_diff,
                gdmem.local_size());
  } else {
    EXPECT_EQ_U(initial_local_capacity + unit_x_lsize_diff,
                gdmem.local_size());
  }
  dash::barrier();
  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "size checks after commit completed");

  // Initialize values in reallocated memory:
  auto lmem = gdmem.lbegin();
  auto lcap = gdmem.local_size();
  for (size_t li = 0; li < lcap; ++li) {
    auto value = 1000 * (dash::myid() + 1) + li;
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                   "setting local offset", li, "at unit", dash::myid(),
                   "value:", value);
    lmem[li] = value;
  }
  dash::barrier();
  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "initialization of local values completed");

  if (dash::myid() == 0) {
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                   "testing basic iterator arithmetic");

    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc", "git_first");
    auto git_first  = gdmem.begin();
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc", "git_second");
    auto git_second = git_first + 1;
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc", "git_remote");
    auto git_remote = git_first + gdmem.local_size() + 1;
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc", "git_last");
    auto git_last   = git_first + gdmem.size() - 1;
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc", "git_end");
    auto git_end    = git_first + gdmem.size();

    dash__unused(git_second);
    dash__unused(git_remote);
    dash__unused(git_last);
    dash__unused(git_end);
  }
  dash::barrier();
  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "testing basic iterator arithmetic completed");

  // Test memory space of units separately:
  for (dash::team_unit_t unit{0}; unit < dash::Team::All().size();
       ++unit) {
    if (dash::myid() != unit) {
      auto unit_git_begin = gdmem.at(unit, 0);
      auto unit_git_end   = gdmem.at(unit, gdmem.local_size(unit));
      auto exp_l_capacity = initial_local_capacity;
      if (unit == 0) {
        exp_l_capacity += unit_0_lsize_diff;
      } else if (unit == 1) {
        exp_l_capacity += unit_1_lsize_diff;
      } else {
        exp_l_capacity += unit_x_lsize_diff;
      }
      DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                     "remote unit:",          unit,
                     "expected local size:",  exp_l_capacity,
                     "gdm.local_size(unit):", gdmem.local_size(unit),
                     "git_end - git_begin:",  unit_git_end - unit_git_begin);
      EXPECT_EQ_U(exp_l_capacity, gdmem.local_size(unit));
      EXPECT_EQ_U(exp_l_capacity, unit_git_end - unit_git_begin);
      int l_idx = 0;
      for(auto it = unit_git_begin; it != unit_git_end; ++it, ++l_idx) {
        DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                       "requesting element at",
                       "local offset", l_idx,
                       "from unit",    unit);
        auto gptr = it.dart_gptr();
        DASH_LOG_TRACE_VAR("GlobDynamicMemTest.UnbalancedRealloc", gptr);

        // request value via DART global pointer:
        value_t dart_gptr_value;
        dart_storage_t ds = dash::dart_storage<value_t>(1);
        dart_get_blocking(&dart_gptr_value, gptr, ds.nelem, ds.dtype);
        DASH_LOG_TRACE_VAR("GlobDynamicMemTest.UnbalancedRealloc",
                           dart_gptr_value);

        // request value via DASH global iterator:
        value_t git_value = *it;
        DASH_LOG_TRACE_VAR("GlobDynamicMemTest.UnbalancedRealloc",
                           git_value);

        value_t expected = 1000 * (unit + 1) + l_idx;
        EXPECT_EQ_U(expected, dart_gptr_value);
        EXPECT_EQ_U(expected, git_value);
      }
    }
  }
  dash::barrier();

  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "testing reverse iteration");

  // Test memory space of all units by iterating global index space:
  dash::team_unit_t unit(dash::Team::All().size() - 1);
  auto local_offset = gdmem.local_size(unit) - 1;
  // Invert order to test reverse iterators:
  auto rgend        = gdmem.rend();
  EXPECT_EQ_U(gdmem.size(), gdmem.rend() - gdmem.rbegin());
  for (auto rgit = gdmem.rbegin(); rgit != rgend; ++rgit) {
    DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                   "requesting element at",
                   "local offset", local_offset,
                   "from unit",    unit);
    value_t expected   = 1000 * (unit + 1) + local_offset;
    value_t rgit_value = *rgit;
    DASH_LOG_TRACE_VAR("GlobDynamicMemTest.UnbalancedRealloc", rgit_value);
    value_t git_value  = *gdmem.at(unit, local_offset);
    DASH_LOG_TRACE_VAR("GlobDynamicMemTest.UnbalancedRealloc", git_value);

    EXPECT_EQ_U(expected, rgit_value);
    EXPECT_EQ_U(expected, git_value);
    if (local_offset == 0 && unit > 0) {
      --unit;
      local_offset = gdmem.local_size(unit);
    }
    --local_offset;
  }

  DASH_LOG_TRACE("GlobDynamicMemTest.UnbalancedRealloc",
                 "testing reverse iteration completed");
}

TEST_F(GlobDynamicMemTest, LocalVisibility)
{
  typedef int value_t;

  if (dash::size() < 2) {
    SKIP_TEST_MSG("Test case requires at least two units");
  }

  LOG_MESSAGE("initializing GlobDynamicMem<T>");

  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  LOG_MESSAGE("initial global capacity: %ld, initial local capacity: %ld",
              initial_global_capacity, initial_local_capacity);
  dash::barrier();

  // Total changes of local capacity:
  int unit_0_lsize_diff =  5;
  int unit_1_lsize_diff = -2;

  if (dash::myid() == 0) {
    // results in 2 buckets to attach, 0 to detach
    gdmem.grow(3);
    gdmem.shrink(2);
    gdmem.grow(5);
    gdmem.shrink(1);
  }
  if (dash::myid() == 1) {
    // results in 0 buckets to attach, 0 to detach
    gdmem.shrink(2);
    gdmem.grow(5);
    gdmem.shrink(2);
    gdmem.shrink(3);
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
  std::string my_host     = dash::util::UnitLocality(dash::myid()).hostname();
  std::string unit_0_host = dash::util::UnitLocality(dash::global_unit_t{0}).hostname();
  std::string unit_1_host = dash::util::UnitLocality(dash::global_unit_t{1}).hostname();

  size_t expected_visible_size = initial_global_capacity;
  size_t expected_global_size  = initial_global_capacity;
  if (dash::myid() == 0) {
    expected_visible_size += unit_0_lsize_diff;
    if (my_host == unit_1_host) {
      LOG_MESSAGE("expected visible size: %ld (locally) or %ld (shmem)",
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
      LOG_MESSAGE("expected visible size: %ld (locally) or %ld (shmem)",
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
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "tests of visible memory size before commit passed");

  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "testing local capacities after grow/shrink");
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
  auto lbegin = gdmem.lbegin();
  for (size_t li = 0; li < gdmem.local_size(); ++li) {
    value_t value = 100 * (dash::myid() + 1) + li;
    DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                   "local[", li, "] =", value);
    *(lbegin + li) = value;
  }

  dash::barrier();
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "tests of local capacities after grow/shrink passed");

  // Memory marked for deallocation is still accessible by other units:

  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "committing global memory");
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "local capacity before commit:",  gdmem.local_size());
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "global capacity before commit:", gdmem.size());
  // Collectively commit changes of local memory allocation to global
  // memory space:
  // register newly allocated local memory and remove local memory marked
  // for deallocation.
  gdmem.commit();
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility", "commit completed");

  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "local capacity after commit:",  gdmem.local_size());
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "global capacity after commit:", gdmem.size());

  // Changes are globally visible now:
  auto expected_global_capacity = initial_global_capacity +
                                  unit_0_lsize_diff + unit_1_lsize_diff;
  EXPECT_EQ_U(expected_global_capacity, gdmem.size());

  if (dash::myid() == 0) {
    DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility", "grow(30)");
    LOG_MESSAGE("grow(30)");
    gdmem.grow(30);
  }
  if (dash::myid() == 1) {
    DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility", "grow(30)");
    gdmem.grow(30);
  }
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "commit, balanced attach");
  gdmem.commit();
  // Capacity changes have been published globally:
  expected_global_capacity += (30 + 30);
  EXPECT_EQ(expected_global_capacity, gdmem.size());

  if (dash::myid() == 0) {
    // resizes attached bucket:
    DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility", "shrink(29)");
    gdmem.shrink(29);
  }
  if (dash::myid() == 1) {
    // marks bucket for detach:
    DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility", "shrink(30)");
    gdmem.shrink(30);
  }
  DASH_LOG_TRACE("GlobDynamicMemTest.LocalVisibility",
                 "commit, unbalanced detach");
  gdmem.commit();
  // Capacity changes have been published globally:
  expected_global_capacity -= (29 + 30);
  EXPECT_EQ(expected_global_capacity, gdmem.size());
}

TEST_F(GlobDynamicMemTest, RemoteAccess)
{
  typedef int value_t;

  if (dash::size() < 3) {
    SKIP_TEST_MSG("Test case requires at least three units");
  }

  /* Illustration of the test case:
   *
   *  unit 0:
   *
   *  size: 10              size: 15                    size: 15
   * .-------------. grow  .-------------.----. init   .--------.----------.
   * |             | ----> |             |    | -----> | 0 1 .. | .. 13 14 |
   * '-------------`       '-------------'-/--'        '---/----'-----/----'
   *                                      /               /          /
   *                                  allocated,      attached,  unattached,
   *                                  locally         globally   locally
   *                                  visible         visible    visible
   *  unit 1:
   *
   *  size: 10              size: 10                    size: 8
   * .-------------. init  .-------------.   shrink    .---------.-----.
   * |             | ----> | 0 1 ... 8 9 | --------->  | 0 1 2 ..| 8 9 |
   * '-------------`       '-------------'             '--/------'--/--'
   *                                                     /         /
   *                                                attached,   attached,
   *                                                globally    visible to
   *                                   |            visible     remote units
   *                                   :
   *                                   '
   * =============================== COMMIT ==================================
   *                                   :
   *                                  \|/
   *  unit 0:                          V      unit 1:
   *
   *  size: 15                                size: 8
   * .----------------------------------.    .-----------------..-----.
   * | 0 1 2 3 4 5 6 ... 10 11 12 13 14 |    | 0 1 2 3 4 5 6 7 || x x |
   * '--------/-------------------------'    '--------/--------''---/-'
   *         /                                       /             /
   *     attached,                               attached,     detached,
   *     globally                                globally      deallocated
   *     visible                                 visible
   */
  size_t initial_local_capacity  = 10;
  size_t initial_global_capacity = dash::size() * initial_local_capacity;
  dash::GlobDynamicMem<value_t> gdmem(initial_local_capacity);

  int unit_0_num_grow   = 5;
  int unit_1_num_shrink = 2;

  if (dash::myid() == 0) {
    gdmem.resize(initial_global_capacity + unit_0_num_grow);
  }

  EXPECT_EQ_U(gdmem.local_size(),
              std::distance(gdmem.lbegin(), gdmem.lend()));

  // Initialize values in local memory:
  auto lbegin = gdmem.lbegin();
  DASH_LOG_TRACE("GlobDynamicMemTest.RemoteAccess",
                 "globmem.lbegin():", lbegin);
  for (size_t li = 0; li < gdmem.local_size(); ++li) {
    value_t value  = (100 * (dash::myid() + 1)) + li;
    DASH_LOG_TRACE("GlobDynamicMemTest.RemoteAccess",
                   "local[", li, "] =", value);
    *(lbegin + li) = value;
  }
  // Shrink after initialization of local values so elements in the locally
  // removed memory segment have meaningful values.
  if (dash::myid() == 1) {
    gdmem.resize(initial_global_capacity - unit_1_num_shrink);
  }

  // Wait for initialization of local values of all units:
  dash::barrier();

  for (dash::team_unit_t u{0}; u < dash::size(); ++u) {
    if (dash::myid() != dash::global_unit_t(u)) {
      size_t  nlocal_expect = initial_local_capacity;
      size_t  nlocal_elem   = gdmem.local_size(u);

      EXPECT_EQ_U(nlocal_expect, nlocal_elem);
      DASH_LOG_DEBUG("GlobDynamicMemTest.RemoteAccess",
                     "requesting element from unit", u,
                     "before commit,", "local capacity:", nlocal_elem);
      for (size_t lidx = 0; lidx < nlocal_elem; ++lidx) {
        // Retreive global pointer from local-to-global address resolution
        // provided my global memory management:
        auto    u_last_elem = gdmem.at(u, lidx);
        value_t expected    = (100 * (u + 1)) + lidx;
        value_t actual;
        // Request value from unit u via dart_get on global pointer:
        dash::get_value(&actual, u_last_elem);
        EXPECT_EQ_U(expected, actual);
      }
    }
  }

  gdmem.commit();

  // Changed sizes of memory spaces are visible to all units after commit:
  EXPECT_EQ_U(initial_local_capacity + unit_0_num_grow,
              gdmem.local_size(dash::team_unit_t{0}));
  EXPECT_EQ_U(initial_local_capacity - unit_1_num_shrink,
              gdmem.local_size(dash::team_unit_t{1}));

  // Validate values after commit:
  for (dash::team_unit_t u{0}; u < dash::size(); ++u) {
    if (dash::myid() != dash::global_unit_t(u)) {
      size_t  nlocal_elem   = gdmem.local_size(u);
      size_t  nlocal_expect = initial_local_capacity;
      if (u == 0) { nlocal_expect += unit_0_num_grow; }
      if (u == 1) { nlocal_expect -= unit_1_num_shrink; }

      EXPECT_EQ_U(nlocal_expect, nlocal_elem);
      DASH_LOG_DEBUG("GlobDynamicMemTest.RemoteAccess",
                     "requesting element from unit", u,
                     "after commit,", "local capacity:", nlocal_elem);
      for (size_t lidx = 0; lidx < nlocal_elem; ++lidx) {
        // Retreive global pointer from local-to-global address resolution
        // Retreive global pointer from local-to-global address resolution
        // provided my global memory management:
        auto    u_last_elem = gdmem.at(u, lidx);
        value_t expected    = (100 * (u + 1)) + lidx;
        value_t actual;
        // Request value from unit u via dart_get on global pointer:
        dash::get_value(&actual, u_last_elem);
        EXPECT_EQ_U(expected, actual);
      }
    }
  }
}
