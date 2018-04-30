
#include "TeamLocalityTest.h"

#include <dash/util/TeamLocality.h>

#include <string>
#include <sstream>
#include <vector>
#include <iostream>


void print_locality_domain(
  std::string                        context,
  const dash::util::LocalityDomain & ld)
{
  if (dash::myid() != 0) {
    return;
  }

  std::string context_pref = "TeamLocalityTest.locality_domain.";
  context_pref += context;

  LOG_MESSAGE("%s: ", context_pref.c_str());

  std::cerr << ld << std::endl;
}

void test_locality_hierarchy_integrity(
  const dash::util::LocalityDomain & ld)
{
  if (ld.dart_type().num_domains == 0) {
    EXPECT_EQ_U(ld.dart_type().children, nullptr);
  } else {
    EXPECT_NE_U(ld.dart_type().children, nullptr);
  }
  for (auto & sd : ld) {
    test_locality_hierarchy_integrity(sd);
  }
}


TEST_F(TeamLocalityTest, GlobalAll)
{
  if (dash::myid().id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG_VAR("TeamLocalityTest.GlobalAll", tloc.domain());

  EXPECT_EQ_U(team, tloc.team());

  DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                 "team all, global domain, units:",
                 tloc.global_units().size());
  EXPECT_EQ_U(team.size(), tloc.global_units().size());

  for (auto & unit : tloc.global_units()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                   "team all, global domain, units[]:", unit);
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                 "team all, global domain, parts:", tloc.parts().size());
  EXPECT_EQ_U(0, tloc.parts().size());
}

TEST_F(TeamLocalityTest, SplitCore)
{
  if (dash::size() < 2) {
    SKIP_TEST();
  }

  dash::Team & team = dash::Team::All();
  int num_split     = std::min(dash::size(), size_t(3));

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team locality in Global domain:");
  DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitCore", tloc.domain());

  // Split via explicit method call:
  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, splitting into", num_split, "Core domains");
  tloc.split(dash::util::Locality::Scope::Core, num_split);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, Core parts:", tloc.parts().size());

  for (auto & part : tloc.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                   "team locality in Core domain:");
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitCore", part);
  }

  dash::barrier();
}

TEST_F(TeamLocalityTest, SplitNUMA)
{
  if (dash::myid().id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "--> initialize TeamLocaliy(Team::All) ...");
  dash::util::TeamLocality tloc(team);
  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "<-- initialized TeamLocaliy(Team::All)");

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "--> selecting domains at NUMA scope ...");
  auto numa_domains = tloc.domain().scope_domains(
                        dash::util::Locality::Scope::NUMA);
  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "<-- number of NUMA domains:", numa_domains.size());

  if (numa_domains.size() < 2) {
    SKIP_TEST_MSG("Test requires at least 2 NUMA domains");
  }

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team locality in Global domain:");
  DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitNUMA", tloc.domain());

  // Split via constructor parameter:
  dash::util::TeamLocality tloc_numa(
      team, dash::util::Locality::Scope::NUMA);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team all, NUMA parts:", tloc_numa.parts().size());

  for (auto & part : tloc_numa.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                   "team locality NUMA domain:");
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitNUMA", part);
  }
}

TEST_F(TeamLocalityTest, GroupUnits)
{
  if (dash::size() < 3) {
    SKIP_TEST_MSG("Test requires at least 3 units");
  }
  if (dash::myid().id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team locality in global domain:");
  print_locality_domain("global", tloc.domain());

  std::vector<dash::global_unit_t> group_1_units;
  std::vector<dash::global_unit_t> group_2_units;
  std::vector<dash::global_unit_t> group_3_units;
  std::vector<std::string> group_1_tags;
  std::vector<std::string> group_2_tags;
  std::vector<std::string> group_3_tags;

  std::vector<dash::global_unit_t> shuffled_unit_ids;
  for (dash::global_unit_t u{0}; u < team.size(); ++u) {
    shuffled_unit_ids.push_back(u);
  }
  std::random_shuffle(shuffled_unit_ids.begin(), shuffled_unit_ids.end());

  // Put the first 2 units in group 1:
  group_1_units.push_back(shuffled_unit_ids.back());
  shuffled_unit_ids.pop_back();
  group_1_units.push_back(shuffled_unit_ids.back());
  shuffled_unit_ids.pop_back();

  group_2_units.push_back(shuffled_unit_ids.back());
  shuffled_unit_ids.pop_back();

  group_3_units = shuffled_unit_ids;

  std::vector<int> unit_core_ids;

  for (dash::global_unit_t u : group_1_units) {
    const auto & unit_loc = tloc.unit_locality(u);
    group_1_tags.push_back(unit_loc.domain_tag());
    unit_core_ids.push_back(unit_loc.hwinfo().core_id);
  }
  for (dash::global_unit_t u : group_2_units) {
    const auto & unit_loc = tloc.unit_locality(u);
    group_2_tags.push_back(unit_loc.domain_tag());
    unit_core_ids.push_back(unit_loc.hwinfo().core_id);
  }
  for (dash::global_unit_t u : group_3_units) {
    const auto & unit_loc = tloc.unit_locality(u);
    group_3_tags.push_back(unit_loc.domain_tag());
    unit_core_ids.push_back(unit_loc.hwinfo().core_id);
  }

  std::sort(unit_core_ids.begin(), unit_core_ids.end());
  if (std::unique(unit_core_ids.begin(), unit_core_ids.end())
      != unit_core_ids.end()) {
    SKIP_TEST_MSG("Multiple units mapped to same core is not supported yet");
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 1:",
                 group_1_units, group_1_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 2:",
                 group_2_units, group_2_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 3:",
                 group_3_units, group_3_tags);

  if (group_1_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_1_tags);
    const auto & group_1 = tloc.group(group_1_tags);
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.GroupUnits", group_1);

    auto group_1_units_actual = group_1.units();
    std::sort(group_1_units.begin(),        group_1_units.end());
    std::sort(group_1_units_actual.begin(), group_1_units_actual.end());

    EXPECT_EQ_U(group_1_units, group_1_units_actual);
  }
  if (group_2_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_2_tags);
    const auto & group_2 = tloc.group(group_2_tags);
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.GroupUnits", group_2);

    auto group_2_units_actual = group_2.units();
    std::sort(group_2_units.begin(),        group_2_units.end());
    std::sort(group_2_units_actual.begin(), group_2_units_actual.end());

    EXPECT_EQ_U(group_2_units, group_2_units_actual);
  }
  if (group_3_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_3_tags);
    const auto & group_3 = tloc.group(group_3_tags);
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.GroupUnits", group_3);

    auto group_3_units_actual = group_3.units();
    std::sort(group_3_units.begin(),        group_3_units.end());
    std::sort(group_3_units_actual.begin(), group_3_units_actual.end());

    EXPECT_EQ_U(group_3_units, group_3_units_actual);
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "Global domain after grouping:");
  DASH_LOG_DEBUG_VAR("TeamLocalityTest.GroupUnits", tloc.domain());

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team all, groups:", tloc.groups().size());

  for (const auto & group : tloc.groups()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                   "team locality group domain: tag:", group->domain_tag());

    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "----------------------");
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.GroupUnits", *group);
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "----------------------");
  }
}

TEST_F(TeamLocalityTest, SplitGroups)
{
  if (dash::size() < 4) {
    SKIP_TEST_MSG("Test requires at least 4 units");
  }
  if (dash::myid().id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups",
                 "team locality in Global domain:");
  DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitGroups", tloc.domain());

  std::vector<dash::global_unit_t> group_1_units;
  std::vector<dash::global_unit_t> group_2_units;
  std::vector<std::string> group_1_tags;
  std::vector<std::string> group_2_tags;

  // Put the first 2 units in group 1:
  group_1_units.push_back(dash::global_unit_t{0});
  group_1_units.push_back(dash::global_unit_t{1});
  // Put every second unit in group 2, starting at rank 2:
  for (dash::global_unit_t u{3}; u < team.size(); u += 2) {
    group_2_units.push_back(u);
  }

  for (dash::global_unit_t u : group_1_units) {
    group_1_tags.push_back(tloc.unit_locality(u).domain_tag());
  }
  for (dash::global_unit_t u : group_2_units) {
    group_2_tags.push_back(tloc.unit_locality(u).domain_tag());
  }

  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group 1:", group_1_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group 2:", group_2_tags);

  std::vector<int> unit_core_ids;

  for (dash::global_unit_t u : group_1_units) {
    const auto & unit_loc = tloc.unit_locality(u);
    group_1_tags.push_back(unit_loc.domain_tag());
    unit_core_ids.push_back(unit_loc.hwinfo().core_id);
  }
  for (dash::global_unit_t u : group_2_units) {
    const auto & unit_loc = tloc.unit_locality(u);
    group_2_tags.push_back(unit_loc.domain_tag());
    unit_core_ids.push_back(unit_loc.hwinfo().core_id);
  }

  std::sort(unit_core_ids.begin(), unit_core_ids.end());
  if (std::unique(unit_core_ids.begin(), unit_core_ids.end())
      != unit_core_ids.end()) {
    SKIP_TEST_MSG("Multiple units mapped to same core is not supported yet");
  }

  if (group_1_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group:", group_1_tags);
    const auto & group_1 = tloc.group(group_1_tags);
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitGroups", group_1);

    // TODO: If requested split was not possible, this yields an incorrect
    //       failure:
    //  EXPECT_EQ_U(group_1_units, group_1.units());
  }
  if (group_2_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group:", group_2_tags);
    const auto & group_2 = tloc.group(group_2_tags);
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitGroups", group_2);

    EXPECT_EQ_U(group_2_units, group_2.units());
  }

  tloc.split_groups();

  for (const auto & part : tloc.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups",
                   "team locality split group:");
    DASH_LOG_DEBUG_VAR("TeamLocalityTest.SplitGroups", part);
  }
}
