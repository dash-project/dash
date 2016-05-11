#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TeamLocalityTest.h"

#include <string>


void print_locality_domain(const dash::util::LocalityDomain & ld)
{
  std::string indent(ld.level() * 4, ' ');

  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "scope:   ",
                 ld.scope());
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "tag:     ",
                 ld.domain_tag());
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "unit ids:",
                 ld.units());
  DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "domains: ",
                 ld.size());

  int d = 0;
  for (auto & domain : ld) {
    DASH_LOG_DEBUG("TeamLocalityTest.print_domain", indent + "[", d, "]:");
    print_locality_domain(domain);
    ++d;
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
                 "team all, global domain, domains:", tloc.domains().size());
  EXPECT_EQ_U(1, tloc.domains().size());

  for (auto & domain : tloc.domains()) {
    DASH_LOG_DEBUG("TeamLocalityTest.GlobalAll", "team locality domain:");
    print_locality_domain(domain);
  }
}

TEST_F(TeamLocalityTest, SplitCore)
{
  dash::Team & team = dash::Team::All();
  int num_split     = std::min<int>(dash::size(), 3);

  dash::util::TeamLocality tloc(team);
  for (auto & domain : tloc.domains()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                   "team locality in Global domain:");
    print_locality_domain(domain);
  }

  // Split via explicit method call:
  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, splitting into", num_split, "Core domains");
  tloc.split(dash::util::Locality::Scope::Core, num_split);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                 "team all, Core domains:", tloc.domains().size());

  for (auto & domain : tloc.domains()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitCore",
                   "team locality in Core domain:");
    print_locality_domain(domain);
  }
}

TEST_F(TeamLocalityTest, SplitNUMA)
{
  dash::Team & team = dash::Team::All();

  // Split via constructor parameter:
  dash::util::TeamLocality tloc(team, dash::util::Locality::Scope::NUMA);

  DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                 "team all, NUMA domains:", tloc.domains().size());

  for (auto & domain : tloc.domains()) {
    DASH_LOG_DEBUG("TeamLocalityTest.SplitNUMA",
                   "team locality NUMA domain:");
    print_locality_domain(domain);
  }
}
