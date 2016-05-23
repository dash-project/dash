#include <libdash.h>
#include <gtest/gtest.h>

#include "LoadBalancePatternTest.h"
#include "TestBase.h"

#include <dash/LoadBalancePattern.h>


TEST_F(LoadBalancePatternTest, LocalSizes)
{
  if (dash::size() < 2) {
    return;
  }

  typedef dash::LoadBalancePattern<1> pattern_t;
  typedef dash::util::TeamLocality    team_loc_t;

  size_t       size = 2017;
  dash::Team & team = dash::Team::All();
  team_loc_t   tloc(team);

  // Initialize identical hwinfo for all units:
  for (dart_unit_t u = 0; u < static_cast<dart_unit_t>(_dash_size); u++) {
    auto & unit_hwinfo      = tloc.unit_locality(u).hwinfo();
    unit_hwinfo.min_threads = 1;
    unit_hwinfo.max_threads = 2;
    unit_hwinfo.min_cpu_mhz = 1200;
    unit_hwinfo.max_cpu_mhz = 1600;
  }

  auto & unit_0_hwinfo      = tloc.unit_locality(0).hwinfo();
  auto & unit_1_hwinfo      = tloc.unit_locality(1).hwinfo();

  // Double min. number of threads and CPU capacity of unit 0:
  unit_0_hwinfo.min_threads = 2;
  unit_0_hwinfo.min_cpu_mhz = unit_0_hwinfo.min_cpu_mhz * 2;
  unit_0_hwinfo.max_cpu_mhz = unit_0_hwinfo.min_cpu_mhz;

  // Half min. number of threads and CPU capacity of unit 1:
  unit_1_hwinfo.min_cpu_mhz = unit_1_hwinfo.min_cpu_mhz / 2;
  unit_1_hwinfo.max_cpu_mhz = unit_1_hwinfo.min_cpu_mhz;

  // Ratio unit 0 CPU capacity / unit 1 CPU capacity:
  double cpu_cap_ratio      = 8.0;
  double cap_balanced       = static_cast<double>(size) /
                                (8 + 1 + (2 * (dash::size() - 2)));

  pattern_t pat(dash::SizeSpec<1>(size), tloc);

  // Test that all elements have been assigned:
  ASSERT_EQ_U(size, pat.size());

  size_t unit_1_lsize_exp   = std::floor(cap_balanced);
  size_t unit_x_lsize_exp   = std::floor(cap_balanced * 2);
  size_t unit_0_lsize_exp   = size - unit_1_lsize_exp -
                                ((_dash_size - 2) * unit_x_lsize_exp);

  DASH_LOG_DEBUG_VAR("LoadBalancePatternTest.LocalSizes", unit_0_lsize_exp);
  DASH_LOG_DEBUG_VAR("LoadBalancePatternTest.LocalSizes", unit_1_lsize_exp);
  DASH_LOG_DEBUG_VAR("LoadBalancePatternTest.LocalSizes", unit_x_lsize_exp);

  EXPECT_EQ_U(cpu_cap_ratio,
              std::floor(
                pat.local_size(0) / pat.local_size(1)));

  EXPECT_EQ_U(unit_0_lsize_exp, pat.local_size(0));
  EXPECT_EQ_U(unit_1_lsize_exp, pat.local_size(1));

  for (dart_unit_t u = 2; u < static_cast<dart_unit_t>(_dash_size); u++) {
    EXPECT_EQ_U(unit_x_lsize_exp, pat.local_size(u));
  }
}
