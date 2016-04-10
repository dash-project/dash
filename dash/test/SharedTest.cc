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

TEST_F(SharedTest, ArrayOfShared)
{
  typedef int                   value_t;
  typedef dash::Shared<value_t> shared_t;

  value_t  my_value  = dash::myid() + 100;
  // setting active unit as owner of own shared value:
  shared_t my_shared = shared_t(dash::myid());
  my_shared.set(my_value);

  dash::Array<shared_t> array_of_shared(dash::size(), { my_shared },
                                        dash::BLOCKED);

  LOG_MESSAGE("writing shared value %d", my_value);
  array_of_shared.local[0].set(my_value);
  array_of_shared.barrier();

  EXPECT_EQ(dash::size(), array_of_shared.size());

  // Log shared values:
  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("SharedTest.ArrayOfShared",
                   "creating local copy of", array_of_shared.size(),
                   "shared values");
    std::vector<value_t> shared_values_copy;
    for (dart_unit_t unit = 0; unit < array_of_shared.size(); ++unit) {
      shared_t shared = array_of_shared[unit];
      value_t  value  = shared.get();
      DASH_LOG_DEBUG("SharedTest.ArrayOfShared",
                     "unit:",  unit,
                     "value:", value);
      shared_values_copy.push_back(value);
    }
    DASH_LOG_DEBUG_VAR("SharedTest.ArrayOfShared", shared_values_copy);
  }
  array_of_shared.barrier();

  for (dart_unit_t unit = 0; unit < array_of_shared.size(); ++unit) {
    LOG_MESSAGE("reading shared value at unit %d", unit);
    value_t  expected = unit + 100;
    shared_t shared   = array_of_shared[unit];
    value_t  actual   = shared.get();
    EXPECT_EQ_U(expected, actual);
    ++unit;
  }
}
