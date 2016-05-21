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

  std::string indent(ld.level() * 4, ' ');

  if (ld.level() == 0) {
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "context: ",
                   context);
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "----------");
  }

  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "scope:   ",
                 static_cast<dart_locality_scope_t>(ld.scope()));
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "rel.idx: ",
                 ld.relative_index());
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "tag:     ",
                 ld.domain_tag());
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "unit ids:",
                 ld.units());

  if (ld.size() > 0) {
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "domains: ",
                   ld.size());
  }

  int d = 0;
  for (auto & domain : ld) {
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "[", d, "]:");
    print_locality_domain(context, domain);
    ++d;
  }
  if (ld.level() == 0) {
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "----------");
  }
}

TEST_F(TeamLocalityTest, GlobalAll)
{
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
}

TEST_F(TeamLocalityTest, SplitNUMA)
{
  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  // Split via constructor parameter:
  dash::util::TeamLocality tloc_numa(
      team, dash::util::Locality::Scope::NUMA);

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
  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team locality in Global domain:");
  print_locality_domain("global", tloc.domain());

  dash::barrier();

  // Split via constructor parameter:
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "Add first group");
  auto & group_1 = tloc.group({ ".0.0.0.0", ".0.0.0.1" });
  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "First group ptr:", &group_1);
  print_locality_domain("group_1", group_1);

  dash::barrier();

  auto subdom_2 = tloc.domain().find(".0.0.1");
  if (subdom_2 != tloc.domain().end() && subdom_2->units().size() > 1) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                   "Add second group");
    auto & group_2 = tloc.group({ ".0.0.1.0", ".0.0.1.1" });
    print_locality_domain("group_2", group_2);
  }
  dash::barrier();

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "Global domain after grouping:");
  print_locality_domain("global", tloc.domain());

  dash::barrier();

  DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                 "team all, groups:", tloc.groups().size());

  for (auto group : tloc.groups()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GroupUnits",
                   "team locality group domain: tag:", group->domain_tag());

    print_locality_domain("Group", *group);
  }
}

