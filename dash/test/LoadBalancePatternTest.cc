#include <libdash.h>
#include <gtest/gtest.h>

#include "LoadBalancePatternTest.h"
#include "TestBase.h"


void mock_team_locality(
  dash::util::TeamLocality & tloc)
{
  auto nunits = tloc.team().size();
  // Initialize identical hwinfo for all units:
  for (dash::team_unit_t u{0}; u < nunits; u++) {
    auto & unit_hwinfo      = tloc.unit_locality(u).hwinfo();
    unit_hwinfo.min_threads = 1;
    unit_hwinfo.max_threads = 2;
    unit_hwinfo.min_cpu_mhz = 1200;
    unit_hwinfo.max_cpu_mhz = 1600;
  }

  auto & unit_0_hwinfo      = tloc.unit_locality(dash::team_unit_t{0}).hwinfo();
  auto & unit_1_hwinfo      = tloc.unit_locality(dash::team_unit_t{1}).hwinfo();

  // Double min. number of threads and CPU capacity of unit 0:
  unit_0_hwinfo.min_threads = 2;
  unit_0_hwinfo.min_cpu_mhz = unit_0_hwinfo.min_cpu_mhz * 2;
  unit_0_hwinfo.max_cpu_mhz = unit_0_hwinfo.min_cpu_mhz;

  // Half min. number of threads and CPU capacity of unit 1:
  unit_1_hwinfo.min_cpu_mhz = unit_1_hwinfo.min_cpu_mhz / 2;
  unit_1_hwinfo.max_cpu_mhz = unit_1_hwinfo.min_cpu_mhz;
}


TEST_F(LoadBalancePatternTest, LocalSizes)
{
  if (_dash_size < 2) {
    LOG_MESSAGE("LoadBalancePatternTest.LocalSizes "
                "requires > 1 units");
    return;
  }

  return; // Temporarily disabled

  typedef dash::LoadBalancePattern<1> pattern_t;
  typedef dash::util::TeamLocality    team_loc_t;

  size_t     size = 2017;
  team_loc_t tloc(dash::Team::All());

  mock_team_locality(tloc);

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
                pat.local_size(dash::team_unit_t{0}) / pat.local_size(dash::team_unit_t{1})));

  EXPECT_EQ_U(unit_0_lsize_exp, pat.local_size(dash::team_unit_t{0}));
  EXPECT_EQ_U(unit_1_lsize_exp, pat.local_size(dash::team_unit_t{1}));

  for (dash::team_unit_t u{2}; u < _dash_size; u++) {
    EXPECT_EQ_U(unit_x_lsize_exp, pat.local_size(u));
  }
}

TEST_F(LoadBalancePatternTest, IndexMapping)
{
  if (_dash_size < 2) {
    LOG_MESSAGE("LoadBalancePatternTest.IndexMapping "
                "requires > 1 units");
    return;
  }

  typedef dash::LoadBalancePattern<1> pattern_t;
  typedef pattern_t::index_type       index_t;
  typedef dash::util::TeamLocality    team_loc_t;

  size_t     size = 27;
  team_loc_t tloc(dash::Team::All());

  mock_team_locality(tloc);

  pattern_t pattern(dash::SizeSpec<1>(size), tloc);

  if (_dash_id == 0) {
    dash::test::print_pattern_mapping(
      "pattern.unit_at", pattern, 2,
      [](const pattern_t & _pattern, int _x) -> dart_unit_t {
          return _pattern.unit_at(std::array<index_t, 1> {{ _x }});
      });
    dash::test::print_pattern_mapping(
      "pattern.at", pattern, 2,
      [](const pattern_t & _pattern, int _x) -> index_t {
          return _pattern.at(std::array<index_t, 1> {{ _x }});
      });
    dash::test::print_pattern_mapping(
      "pattern.block_at", pattern, 2,
      [](const pattern_t & _pattern, int _x) -> index_t {
          return _pattern.block_at(
                   std::array<index_t, 1> {{ _x }});
      });
    dash::test::print_pattern_mapping(
      "pattern.block.offset", pattern, 2,
      [](const pattern_t & _pattern, int _x) -> std::string {
          auto block_idx = _pattern.block_at(
                             std::array<index_t, 1> {{ _x }});
          auto block_vs  = _pattern.block(block_idx);
          std::ostringstream ss;
          ss << block_vs.offset(0);
          return ss.str();
      });
    dash::test::print_pattern_mapping(
      "pattern.local_index", pattern, 2,
      [](const pattern_t & _pattern, int _x) -> index_t {
          return _pattern.local_index(
                   std::array<index_t, 1> {{ _x }}).index;
      });
  }

  size_t  total_size = 0;
  index_t g_index    = 0;
  for (dash::team_unit_t u{0}; u < _dash_size; ++u) {
    index_t l_size = pattern.local_size(u);
    for (index_t li = 0; li < l_size; li++) {
      EXPECT_EQ_U(li, pattern.at(g_index));
      EXPECT_EQ_U(li, pattern.local_index(
                        std::array<index_t, 1> {{ g_index }}
                      ).index);
      EXPECT_EQ_U(u,  pattern.unit_at(g_index));
      EXPECT_EQ_U(u,  pattern.block_at(
                        std::array<index_t, 1> {{ g_index }}));
      g_index++;
    }
    total_size += l_size;
  }
  EXPECT_EQ_U(pattern.size(), total_size);
}
