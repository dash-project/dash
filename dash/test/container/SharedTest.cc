
#include "SharedTest.h"

#include <dash/Shared.h>
#include <dash/Atomic.h>

#include <iostream>
#include <sstream>


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

  value_t  value_a_init =  100;
  value_t  value_b_init =  200;
  value_t  value_a      = 1000;
  value_t  value_b      = 2000;
  dash::team_unit_t l_owner_a(owner_a);
  dash::team_unit_t l_owner_b(owner_b);

  // Initialize shared values:
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "initialize shared value at unit", owner_a, "(a)",
                 "with", value_a_init);
  shared_t shared_at_a(value_a_init, l_owner_a);
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "initialize shared value at unit", owner_b, "(b)",
                 "with", value_b_init);
  shared_t shared_at_b(value_b_init, l_owner_b);

  value_t get_a_init = static_cast<value_t>(shared_at_a.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_a, " (a):", get_a_init);
  value_t get_b_init = static_cast<value_t>(shared_at_b.get());
  DASH_LOG_DEBUG("SharedTest.SpecifyOwner",
                 "shared value at unit", owner_b, " (b):", get_b_init);
  EXPECT_EQ_U(value_a_init, get_a_init);
  EXPECT_EQ_U(value_b_init, get_b_init);

  // Wait for validation of read shared values at all units before setting
  // new values:
  shared_at_a.barrier();
  shared_at_b.barrier();

  // Overwrite shared values local:
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

  // Overwrite shared values remote:
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

struct CompositeValue {
  char  c1;
  char  c2;
  char  c3;
  char  c4;
  short s;

  constexpr bool operator==(const CompositeValue & rhs) const {
    return (c1 == rhs.c1 &&
            c2 == rhs.c2 &&
            c3 == rhs.c3 &&
            c4 == rhs.c4 &&
            s  == rhs.s);
  }
  constexpr bool operator!=(const CompositeValue & rhs) const {
    return !(*this == rhs);
  }
};

std::ostream & operator<<(
    std::ostream & os,
    const CompositeValue & cv)
{
  std::ostringstream ss;
  ss << "composite_value_t("
     << cv.c1 << "," << cv.c2 << "," << cv.c3 << "," << cv.c4 << ","
     << cv.s << ")";
  return operator<<(os, ss.str());
}

TEST_F(SharedTest, CompositeValue)
{
  typedef CompositeValue        value_t;
  typedef dash::Shared<value_t> shared_t;

  shared_t shared;
  value_t  init_val { 'a', 'b', 'c', 'd',
                      static_cast<short>(-1) };
  value_t  my_val   { 'a', 'b', 'c', 'd',
                      static_cast<short>(1 + dash::myid()) };
  value_t  exp_val  { 'a', 'b', 'c', 'd',
                      static_cast<short>(dash::size()) };

  shared = shared_t(init_val);
  shared.barrier();

  value_t shared_init = shared.get();
  EXPECT_EQ_U(init_val, shared_init);

  shared.barrier();

  if (dash::myid().id == dash::size() - 1) {
    shared.set(my_val);
  }
  shared.barrier();

  value_t shared_val = shared.get();
  EXPECT_EQ_U(exp_val, shared_val);
}

TEST_F(SharedTest, AtomicAdd)
{
  typedef int                                   value_t;
  typedef dash::Shared< dash::Atomic<value_t> > shared_t;

  if (dash::size() < 2) {
    SKIP_TEST();
  }

  shared_t shared;
  value_t  init_val = 123;
  value_t  my_val   = 1 + dash::myid();

  if (dash::myid().id == 0) {
    shared.set(init_val);
  }
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 0");
  shared.barrier();

  EXPECT_EQ_U(init_val, static_cast<value_t>(shared.get()));
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.barrier - 1");
  shared.barrier();

  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "sleep");
  sleep(1);
  DASH_LOG_DEBUG("SharedTest.AtomicAdd", "shared.get().add");
  shared.get().add(my_val);
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

TEST_F(SharedTest, AtomicMinMax)
{
  using value_t  = int32_t;
  using shared_t = dash::Shared<dash::Atomic<value_t>>;

  auto&    team = dash::Team::All();
  shared_t g_min(std::numeric_limits<value_t>::max(), dash::team_unit_t{0}, team);
  shared_t g_max(std::numeric_limits<value_t>::min(), dash::team_unit_t{0}, team);

  auto const start_min = static_cast<value_t>(g_min.get());
  auto const start_max = static_cast<value_t>(g_max.get());

  EXPECT_GE_U(start_min, 0);
  EXPECT_LE_U(start_max, std::numeric_limits<value_t>::max());

  team.barrier();

  g_min.get().fetch_op(dash::min<value_t>(), 0);
  g_max.get().fetch_op(dash::max<value_t>(), std::numeric_limits<value_t>::max());

  team.barrier();

  auto const min = static_cast<value_t>(g_min.get());
  auto const max = static_cast<value_t>(g_max.get());

  EXPECT_EQ_U(0, min);
  EXPECT_EQ_U(std::numeric_limits<value_t>::max(), max);
}
