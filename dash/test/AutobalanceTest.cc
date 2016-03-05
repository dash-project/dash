#include <libdash.h>
#include <array>
#include <sstream>

#include "TestBase.h"
#include "AutobalanceTest.h"

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

TEST_F(AutobalanceTest, Factorize)
{
  size_t number  = 2 * 2 * 2 * 3 * 3 * 13 * 17;
  auto   factors = dash::math::factorize(number);

  DASH_LOG_TRACE("AutobalanceTest::Factorize",
                 "factors of", number, ":", factors);

  // Number of keys (primes):
  ASSERT_EQ(4,  factors.size());

  ASSERT_TRUE(factors.count(11) == 0);
  ASSERT_TRUE(factors.count(13) == 1);
  ASSERT_TRUE(factors.count(13) == 1);

  ASSERT_EQ(3,  factors[2]);
  ASSERT_EQ(2,  factors[3]);
  ASSERT_EQ(1,  factors[13]);
  ASSERT_EQ(1,  factors[17]);

  auto kv = factors.begin();
  ASSERT_EQ(2,  (kv++)->first);
  ASSERT_EQ(3,  (kv++)->first);
  ASSERT_EQ(13, (kv++)->first);
  ASSERT_EQ(17, (kv++)->first);

  size_t number_kp  = 7 * 7 * 2081 * 2083;
  auto   factors_kp = dash::math::factorize(number_kp);

  DASH_LOG_TRACE("AutobalanceTest::Factorize",
                 "factors of", number_kp, ":", factors_kp);

  ASSERT_EQ(2,  factors_kp[7]);
  ASSERT_EQ(1,  factors_kp[2081]);
  ASSERT_EQ(1,  factors_kp[2083]);
}

TEST_F(AutobalanceTest, BalanceExtents)
{
  std::array<int, 2> org_extents;
  std::array<int, 2> bal_extents;
  std::array<int, 2> exp_extents;
  std::set<int>      blocking;

  int size = 2 * 5 * 5 * 11 * 19;

  org_extents = {{ size, 1 }};
  bal_extents = dash::math::balance_extents(org_extents);
  exp_extents = {{ 16, 80 }};
  DASH_LOG_TRACE_VAR("AutobalanceTest::BalanceExtents", org_extents);
  DASH_LOG_TRACE_VAR("AutobalanceTest::BalanceExtents", bal_extents);

  org_extents = {{ size, 1 }};
  blocking.insert(70); // unmatched block size
  blocking.insert(50); // matching block size
  blocking.insert(11); // matching block size, but too small
  bal_extents = dash::math::balance_extents(org_extents, blocking);
  exp_extents = {{ 50, size / 50  }};

  DASH_LOG_TRACE_VAR("AutobalanceTest::BalanceExtents", org_extents);
  DASH_LOG_TRACE_VAR("AutobalanceTest::BalanceExtents", bal_extents);

#if 0
  ASSERT_TRUE((exp_extents[0] == bal_extents[0] &&
               exp_extents[1] == bal_extents[1]) ||
              (exp_extents[0] == bal_extents[1] &&
               exp_extents[1] == bal_extents[0]));
#endif
}
