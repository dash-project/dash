
#include "TeamSpecTest.h"

#include <dash/TeamSpec.h>
#include <dash/Team.h>
#include <dash/Distribution.h>

#include <array>
#include <numeric>
#include <functional>


TEST_F(TeamSpecTest, DefaultConstrutor)
{
  int num_units = dash::Team::All().size();
  // Default constructor: All units in first dimension
  EXPECT_EQ_U(num_units, dash::TeamSpec<1>().size());
  EXPECT_EQ_U(1, dash::TeamSpec<1>().rank());
  if (num_units >= 2) {
    EXPECT_EQ_U(num_units, dash::TeamSpec<2>().size());
    EXPECT_EQ_U(num_units, dash::TeamSpec<2>().extent(0));
    EXPECT_EQ_U(1, dash::TeamSpec<2>().extent(1));
    EXPECT_EQ_U(1, dash::TeamSpec<2>().rank());
  }
  if (num_units >= 4) {
    EXPECT_EQ_U(num_units, dash::TeamSpec<4>().size());
    EXPECT_EQ_U(num_units, dash::TeamSpec<4>().extent(0));
    EXPECT_EQ_U(1, dash::TeamSpec<4>().extent(1));
    EXPECT_EQ_U(1, dash::TeamSpec<4>().extent(2));
    EXPECT_EQ_U(1, dash::TeamSpec<4>().extent(3));
    EXPECT_EQ_U(1, dash::TeamSpec<4>().rank());
  }
}

TEST_F(TeamSpecTest, TeamAndDistributionConstrutor)
{
  int num_units = dash::Team::All().size();
  // Team distributed in second dimension (y):
  dash::DistributionSpec<3> dist_blocked_y(
    dash::NONE, dash::BLOCKED, dash::NONE);
  dash::TeamSpec<3> ts_blocked_y(dist_blocked_y, dash::Team::All());
  EXPECT_EQ_U(num_units, ts_blocked_y.size());
  EXPECT_EQ_U(1, ts_blocked_y.extent(0));
  EXPECT_EQ_U(num_units, ts_blocked_y.extent(1));
  EXPECT_EQ_U(1, ts_blocked_y.extent(2));

  // Team distributed in third dimension (z):
  dash::DistributionSpec<3> dist_blocked_z(
    dash::NONE, dash::NONE, dash::BLOCKED);
  dash::TeamSpec<3> ts_blocked_z(dist_blocked_z, dash::Team::All());
  EXPECT_EQ_U(num_units, ts_blocked_z.size());
  EXPECT_EQ_U(1, ts_blocked_z.extent(0));
  EXPECT_EQ_U(1, ts_blocked_z.extent(1));
  EXPECT_EQ_U(num_units, ts_blocked_z.extent(2));
}

TEST_F(TeamSpecTest, ExtentAdjustingConstrutor)
{
  int num_units = dash::Team::All().size();
  dash::DistributionSpec<3> dist_blocked_y(
    dash::NONE, dash::BLOCKED, dash::NONE);

  // Test if extents of default-constructed team spec will
  // be adjusted according to distribution spec:
  dash::TeamSpec<3> ts_default;
  EXPECT_EQ_U(dash::Team::All().size(), ts_default.size());
  // Splitting teams in consecutive test runs not supported
  // for now
  // dash::Team & team_split = dash::Team::All().split(2);
  dash::TeamSpec<3> ts_adjusted(
    // Has extents [n,1,1]
    ts_default,
    // NONE, BLOCKED, NONE -> will adjust to extents [1,n,1]
    dist_blocked_y,
    dash::Team::All());
  EXPECT_EQ_U(1, ts_adjusted.extent(1));
  EXPECT_EQ_U(num_units, ts_adjusted.extent(0));
  EXPECT_EQ_U(1, ts_adjusted.extent(2));
}

TEST_F(TeamSpecTest, Ranks)
{
  DASH_TEST_LOCAL_ONLY();

  dash::TeamSpec<2> teamspec(dash::Team::All());
  ASSERT_EQ(1,            teamspec.rank());
  ASSERT_EQ(dash::size(), teamspec.num_units(0));
  ASSERT_EQ(1,            teamspec.num_units(1));
  ASSERT_EQ(dash::size(), teamspec.size());

}
