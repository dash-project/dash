#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "AtomicTest.h"

#include <vector>
#include <algorithm>
#include <numeric>


TEST_F(AtomicTest, FetchAndOp)
{
  typedef size_t value_t;

  value_t     val_init  = 100;
  dash::team_unit_t owner(dash::size() - 1);
  dash::Shared<value_t> shared(owner);

  if (dash::myid() == 0) {
    shared.set(val_init);
  }
  // wait for initialization:
  dash::barrier();

  dash::Atomic<value_t> atomic(shared);

  atomic.fetch_and_add(2);
  // wait for completion of all atomic operations:
  dash::barrier();

  // incremented by 2 by every unit:
  value_t val_expect   = val_init + (dash::size() * 2);
  value_t a_val_actual = atomic.get();
  value_t s_val_actual = shared.get();
  EXPECT_EQ_U(val_expect, a_val_actual);
  EXPECT_EQ_U(val_expect, s_val_actual);

  dash::barrier();
}

TEST_F(AtomicTest, ArrayElements)
{
  typedef int value_t;

  dash::Array<value_t> array(dash::size());
  value_t my_val = dash::myid() + 1;
  array.local[0] = my_val;
  DASH_LOG_TRACE("AtomicTest.ArrayElement", "barrier #0");
  array.barrier();

  value_t expect_init_acc = (dash::size() * (dash::size() + 1)) / 2;
  if (dash::myid() == 0) {
    // Create local copy for logging:
    value_t *            l_copy = new value_t[array.size()];
    std::vector<value_t> v_copy(array.size());
    dash::copy(array.begin(), array.end(), l_copy);
    std::copy(l_copy, l_copy + array.size(), v_copy.begin());
    DASH_LOG_DEBUG_VAR("AtomicTest.ArrayElement", v_copy);

    value_t actual_init_acc = std::accumulate(
                                l_copy, l_copy + array.size(), 0);
    EXPECT_EQ_U(expect_init_acc, actual_init_acc);
    delete[] l_copy;
  }
  DASH_LOG_TRACE("AtomicTest.ArrayElements", "barrier #1");
  array.barrier();

  dash::team_unit_t remote_prev(dash::myid() == 0
                            ? dash::size() - 1
                            : dash::myid() - 1);
  dash::team_unit_t remote_next(dash::myid() == dash::size() - 1
                            ? 0
                            : dash::myid() + 1);
  // Add own value to previous and next unit in the array's iteration order.
  // In effect, sum of all array values should have tripled.
  DASH_LOG_TRACE("AtomicTest.ArrayElements",
                 "prev: array @ unit(", remote_prev, ") +=", my_val);
  dash::Atomic<value_t>(array[remote_prev]).add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements",
                 "next: array @ unit(", remote_next, ") +=", my_val);
  dash::Atomic<value_t>(array[remote_next]).fetch_and_add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements", "barrier #2");
  array.barrier();

  value_t expect_local = my_val + remote_prev + 1 + remote_next + 1;
  value_t actual_local = array.local[0];
  EXPECT_EQ(expect_local, actual_local);

  if (dash::myid() == 0) {
    // Create local copy for logging:
    value_t *            l_copy = new value_t[array.size()];
    std::vector<value_t> v_copy(array.size());
    dash::copy(array.begin(), array.end(), l_copy);
    std::copy(l_copy, l_copy + array.size(), v_copy.begin());
    DASH_LOG_DEBUG_VAR("AtomicTest.ArrayElements", v_copy);

    value_t expect_res_acc = expect_init_acc * 3;
    value_t actual_res_acc = std::accumulate(
                               l_copy, l_copy + array.size(), 0);
    EXPECT_EQ_U(expect_res_acc, actual_res_acc);
    delete[] l_copy;
  }
}
