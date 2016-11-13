#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TeamLocalityTest.h"

#include <string>


void print_locality_domain(
  std::string                        context,
  const dash::util::LocalityDomain & ld)
{
  if (dash::myid() != 0) {
    return;
  }

  std::string context_pref = "TeamLocalityTest.print_domain.";
  context_pref += context;
  DASH_LOG_DEBUG(context_pref.c_str(), ld);
}


TEST_F(TeamLocalityTest, GlobalAll)
{
  if (_dash_id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  EXPECT_EQ_U(team, tloc.team());

  DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                 "team all, global domain, units:", tloc.units().size());
  EXPECT_EQ_U(team.size(), tloc.units().size());

  for (auto unit : tloc.units()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                   "team all, global domain, units[]:", unit);
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll",
                 "team all, global domain, parts:", tloc.parts().size());
  EXPECT_EQ_U(0, tloc.parts().size());

  print_locality_domain("global", tloc.domain());
}

TEST_F(TeamLocalityTest, SplitCore)
{
  if (_dash_size < 2) {
    SKIP_TEST();
  }

  dash::Team & team = dash::Team::All();
  int num_split     = std::min<int>(dash::size(), 3);

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  // Split via explicit method call:
  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, splitting into", num_split, "Core domains");
  tloc.split(dash::util::Locality::Scope::Core, num_split);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, Core parts:", tloc.parts().size());

  for (auto & part : tloc.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                   "team locality in Core domain:");
    print_locality_domain("CORE split", part);
  }

  dash::barrier();
}

TEST_F(TeamLocalityTest, SplitNUMA)
{
  if (_dash_id != 0) {
    SKIP_TEST();
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  // Split via constructor parameter:
  dash::util::TeamLocality tloc_numa(
      team, dash::util::Locality::Scope::Cache);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team all, NUMA parts:", tloc_numa.parts().size());

  for (auto & part : tloc_numa.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                   "team locality NUMA domain:");
    print_locality_domain("NUMA split", part);
  }
}

TEST_F(TeamLocalityTest, GroupUnits)
{
  SKIP_TEST();

  if (dash::size() < 4) {
    return;
  }
  if (_dash_id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  std::vector<dart_unit_t> group_1_units;
  std::vector<dart_unit_t> group_2_units;
  std::vector<dart_unit_t> group_3_units;
  std::vector<std::string> group_1_tags;
  std::vector<std::string> group_2_tags;
  std::vector<std::string> group_3_tags;

  // Put the first 2 units in group 1:
  group_1_units.push_back(0);
  group_1_units.push_back(1);
  // Put every third unit in group 2, starting at rank 3:
  for (size_t u = 3; u < dash::size(); u += 3) {
    group_2_units.push_back(u);
  }
  // Put every second unit in group 3, starting at center:
  for (size_t u = dash::size() / 2; u < dash::size(); u += 2) {
    // Domains must not be members of more than one group:
    if (u % 3 != 0) {
      group_3_units.push_back(u);
    }
  }

  for (dart_unit_t u : group_1_units) {
    group_1_tags.push_back(tloc.unit_locality(u).domain_tag());
  }
  for (dart_unit_t u : group_2_units) {
    group_2_tags.push_back(tloc.unit_locality(u).domain_tag());
  }
  for (dart_unit_t u : group_3_units) {
    group_3_tags.push_back(tloc.unit_locality(u).domain_tag());
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 1:", group_1_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 2:", group_2_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group 3:", group_3_tags);

  if (group_1_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_1_tags);
    auto & group_1 = tloc.group(group_1_tags);
    print_locality_domain("group_1", group_1);

    // TODO: If requested split was not possible, this yields an incorrect
    //       failure:
    //  EXPECT_EQ_U(group_1_units, group_1.units());
  }
  if (group_2_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_2_tags);
    auto & group_2 = tloc.group(group_2_tags);
    print_locality_domain("group_2", group_2);

    EXPECT_EQ_U(group_2_units, group_2.units());
  }
  if (group_3_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "group:", group_3_tags);
    auto & group_3 = tloc.group(group_3_tags);
    print_locality_domain("group_3", group_3);

    EXPECT_EQ_U(group_3_units, group_3.units());
  }

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "Global domain after grouping:");
  print_locality_domain("global", tloc.domain());

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team all, groups:", tloc.groups().size());

  for (auto group : tloc.groups()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                   "team locality group domain: tag:", group->domain_tag());

    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "----------------------");
    print_locality_domain("Group", *group);
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits", "----------------------");
  }
}

TEST_F(TeamLocalityTest, SplitGroups)
{
  SKIP_TEST();

  if (dash::size() < 4) {
    return;
  }
  if (_dash_id != 0) {
    return;
  }

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  std::vector<dart_unit_t> group_1_units;
  std::vector<dart_unit_t> group_2_units;
  std::vector<std::string> group_1_tags;
  std::vector<std::string> group_2_tags;

  // Put the first 2 units in group 1:
  group_1_units.push_back(0);
  group_1_units.push_back(1);
  // Put every second unit in group 2, starting at rank 2:
  for (size_t u = 3; u < dash::size(); u += 2) {
    group_2_units.push_back(u);
  }

  for (dart_unit_t u : group_1_units) {
    group_1_tags.push_back(tloc.unit_locality(u).domain_tag());
  }
  for (dart_unit_t u : group_2_units) {
    group_2_tags.push_back(tloc.unit_locality(u).domain_tag());
  }

  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group 1:", group_1_tags);
  DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group 2:", group_2_tags);

  if (group_1_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group:", group_1_tags);
    auto & group_1 = tloc.group(group_1_tags);
    print_locality_domain("group_1", group_1);

    // TODO: If requested split was not possible, this yields an incorrect
    //       failure:
    //  EXPECT_EQ_U(group_1_units, group_1.units());
  }
  if (group_2_tags.size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups", "group:", group_2_tags);
    auto & group_2 = tloc.group(group_2_tags);
    print_locality_domain("group_2", group_2);

    EXPECT_EQ_U(group_2_units, group_2.units());
  }

  tloc.split_groups();

  for (auto part : tloc.parts()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitGroups",
                   "team locality split group:");
    print_locality_domain("Group split", part);
  }
}
