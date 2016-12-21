#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "SharedTest.h"


TEST_F(SharedTest, SingleWriteMultiRead)
{
  typedef int value_t;

  value_t shared_value_1 = 123;
  value_t shared_value_2 = 234;
  dash::Shared<value_t> shared;

  // Set initial shared value:
  if (dash::myid() == 0) {
    LOG_MESSAGE("write first shared value: %d", shared_value_1);
    shared.set(shared_value_1);
  }
  dash::barrier();
  value_t actual_1 = static_cast<value_t>(shared.get());
  LOG_MESSAGE("read first shared value: %d", actual_1);
  EXPECT_EQ_U(shared_value_1, actual_1);
  // Wait for validation at all units
  dash::barrier();

  if (dash::size() < 2) {
    return;
  }

  // Overwrite shared value:
  if (dash::myid() == 1) {
    LOG_MESSAGE("write second shared value: %d", shared_value_2);
    shared.set(shared_value_2);
  }
  dash::barrier();
  value_t actual_2 = static_cast<value_t>(shared.get());
  LOG_MESSAGE("read second shared value: %d", actual_2);
  EXPECT_EQ_U(shared_value_2, actual_2);
}

TEST_F(SharedTest, SpecifyOwner)
{
  typedef int                   value_t;
  typedef dash::Shared<value_t> shared_t;

  if (dash::size() < 2) {
    SKIP_TEST();
  }

  dash::global_unit_t owner_a(dash::size() < 3
                         ? 0
                         : dash::size() / 2);
  dash::global_unit_t owner_b(dash::size() - 1);

  value_t  value_a     = 1000;
  value_t  value_b     = 2000;
  dash::team_unit_t l_owner_a(owner_a);
  dash::team_unit_t l_owner_b(owner_b);
  shared_t shared_at_a(l_owner_a);
  shared_t shared_at_b(l_owner_b);

  // Initialize shared values:
  if (dash::myid() == owner_a) {
    DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                   "setting shared value at unit", owner_a, "(a)",
                   "to", value_a);
    shared_at_a.set(value_a);
  }
  else if (dash::myid() == owner_b) {
    DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                   "setting shared value at unit", owner_b, "(b)",
                   "to", value_b);
    shared_at_b.set(value_b);
  }
  shared_at_a.barrier();
  shared_at_b.barrier();

  value_t get_a = static_cast<value_t>(shared_at_a.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_a, " (a):", get_a);
  value_t get_b = static_cast<value_t>(shared_at_b.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_b, " (b):", get_b);
  EXPECT_EQ_U(value_a, get_a);
  EXPECT_EQ_U(value_b, get_b);

  // Wait for validation of read shared values at all units before setting
  // new values:
  shared_at_a.barrier();
  shared_at_b.barrier();

  // Overwrite shared values:
  if (dash::myid() == owner_a) {
    DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                   "setting shared value at unit", owner_b, "(b)",
                   "to", value_a);
    shared_at_b.set(value_a);
  }
  else if (dash::myid() == owner_b) {
    DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                   "setting shared value at unit", owner_a, "(a)",
                   "to", value_b);
    shared_at_a.set(value_b);
  }
  shared_at_a.barrier();
  shared_at_b.barrier();

  value_t new_a = static_cast<value_t>(shared_at_a.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_a, " (a):", new_a);
  value_t new_b = static_cast<value_t>(shared_at_b.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_b, " (b):", new_b);
  EXPECT_EQ_U(value_b, new_a);
  EXPECT_EQ_U(value_a, new_b);
}

TEST_F(SharedTest, AtomicAdd)
{
  typedef int                   value_t;
  typedef dash::Shared<value_t> shared_t;

  if (dash::size() < 2) {
    SKIP_TEST();
  }

  shared_t shared;
  value_t  init_val = 123;
  value_t  my_val   = 1 + dash::myid();

  if (_dash_id == 0) {
    shared.set(init_val);
  }
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 0");
  shared.barrier();

  EXPECT_EQ_U(init_val, static_cast<value_t>(shared.get()));
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 1");
  shared.barrier();

  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "sleep");
  sleep(3);
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.atomic.add");
  shared.atomic.add(my_val);
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 2");
  shared.barrier();

  // Expected total is Gaussian sum:
  value_t exp_acc = init_val + ((dash::size() + 1) * dash::size()) / 2;
  value_t actual  = shared.get();

  EXPECT_EQ_U(exp_acc, actual);

  // Ensure completion of test at all units before destroying shared
  // variable:
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 3");
  shared.barrier();
}
