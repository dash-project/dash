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

TEST_F(AutobalanceTest, BalanceTeamSpecNUMA)
{
  typedef dash::default_size_t     extent_t;
  typedef std::array<extent_t, 2>  extents_t;

  size_t size_base    = 1680;
  int    size_exp_max = 7;

  const extent_t n_numa_per_node  =  2;
  const extent_t n_cores_per_node = 28;

  if (dash::myid() != 0) {
    return;
  }

  std::vector<extents_t> exp_team_extents;
  // For node-level team domain, units should be grouped by
  // NUMA domains (unit grid of <num_numa> x <num_units / num_numa>
  // if no square arrangement is possible):
  exp_team_extents.push_back({{ 2,  2 }}); //  4 units
  exp_team_extents.push_back({{ 2,  4 }}); //  8 units
  exp_team_extents.push_back({{ 2,  6 }}); // 12 units
  exp_team_extents.push_back({{ 4,  4 }}); // 16 units
  exp_team_extents.push_back({{ 2, 10 }}); // 20 units
  exp_team_extents.push_back({{ 2, 12 }}); // 24 units
  exp_team_extents.push_back({{ 2, 14 }}); // 28 units

  // Test for all combinations (team size x data extents):
  extents_t exp_extents;
  for (size_t u = 0; u < exp_team_extents.size(); ++u) {
    for (int s = 0; s < size_exp_max; ++s) {
      auto size_d    = size_base * std::pow(2, s);
      dash::SizeSpec<2> sizespec(size_d, size_d);
      exp_extents    = exp_team_extents[u];
      int  num_units = exp_extents[0] * exp_extents[1];
      DASH_LOG_TRACE("AutobalanceTest::BalanceTeamSpec",
                     "testing balancing of", num_units, "units",
                     "for size ", size_d, "x", size_d);
      auto teamspec  = dash::make_team_spec<
                         dash::summa_pattern_partitioning_constraints,
                         dash::summa_pattern_mapping_constraints,
                         dash::summa_pattern_layout_constraints >(
                           sizespec,
                           num_units,
                           1, n_numa_per_node, n_cores_per_node);
      auto bal_extents = teamspec.extents();
      DASH_LOG_TRACE("AutobalanceTest::BalanceTeamSpec",
                     "balanced", num_units, "units",
                     "for size ", size_d, "x", size_d,
                     ":", bal_extents);

      EXPECT_EQ(num_units, teamspec.size());
      if ((exp_extents[0] != bal_extents[0] ||
           exp_extents[1] != bal_extents[1]) &&
          (exp_extents[0] != bal_extents[1] ||
           exp_extents[1] != bal_extents[0])) {
        EXPECT_EQ(exp_extents, bal_extents);
      }
    }
  }
}

TEST_F(AutobalanceTest, BalanceTeamSpecNodes)
{
  typedef dash::default_size_t     extent_t;
  typedef std::array<extent_t, 2>  extents_t;

  if (dash::myid() != 0) {
    return;
  }

  const extent_t n_numa_per_node  =  4;
  const extent_t n_cores_per_node = 28;

  dash::util::UnitLocality uloc(dash::myid());
  uloc.hwinfo().num_numa  = n_numa_per_node;
  uloc.hwinfo().num_cores = n_cores_per_node;


  std::vector<extents_t> exp_team_extents;
  exp_team_extents.push_back({{ 28,  4 }});
  exp_team_extents.push_back({{ 28,  8 }});
  exp_team_extents.push_back({{ 28, 16 }});
#if 1
  exp_team_extents.push_back({{ 28, 32 }});
#else
  // Actual optimum for SUMMA on SuperMUC:
  exp_team_extents.push_back({{ 16, 56 }});
#endif
  exp_team_extents.push_back({{ 32, 56 }});
  exp_team_extents.push_back({{ 64, 56 }});

  // Test for all combinations (team size x data extents):
  extents_t exp_extents;
  for (size_t n = 0; n < exp_team_extents.size(); ++n) {
    exp_extents    = exp_team_extents[n];
    int  num_units = exp_extents[0] * exp_extents[1];
    auto n_nodes   = num_units / n_cores_per_node;

    auto size_d    = 57344;
    dash::SizeSpec<2> sizespec(size_d, size_d);
    DASH_LOG_TRACE("AutobalanceTest::BalanceTeamSpec",
                   "testing balancing of", num_units, "units",
                   "for size ", size_d, "x", size_d);
    auto teamspec  = dash::make_team_spec<
                       dash::summa_pattern_partitioning_constraints,
                       dash::summa_pattern_mapping_constraints,
                       dash::summa_pattern_layout_constraints >(
                         sizespec,
                         num_units,
                         n_nodes, n_numa_per_node, n_cores_per_node);
    auto bal_extents = teamspec.extents();
    DASH_LOG_TRACE("AutobalanceTest::BalanceTeamSpec",
                   "balanced", num_units, "units",
                   "for size ", size_d, "x", size_d,
                   ":", bal_extents);

    EXPECT_EQ(num_units, teamspec.size());
    if ((exp_extents[0] != bal_extents[0] ||
         exp_extents[1] != bal_extents[1]) &&
        (exp_extents[0] != bal_extents[1] ||
         exp_extents[1] != bal_extents[0])) {
      EXPECT_EQ(exp_extents, bal_extents);
    }
  }
}
