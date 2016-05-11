#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "TeamLocalityTest.h"


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
}

