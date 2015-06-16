#include <libdash.h>
#include <array>
#include <numeric>
#include <functional>
#include "TestBase.h"
#include "TeamSpecTest.h"

TEST_F(TeamSpecTest, DefaultConstrutor) {
  int num_units = dash::Team::All().size();
  // Default constructor: All units in first dimension
  EXPECT_EQ(num_units, dash::TeamSpec<1>().size());
  EXPECT_EQ(1, dash::TeamSpec<1>().rank());
  if (num_units >= 2) {
    EXPECT_EQ(num_units, dash::TeamSpec<2>().size());
    EXPECT_EQ(num_units, dash::TeamSpec<2>().extent(0));
    EXPECT_EQ(1, dash::TeamSpec<2>().extent(1));
    EXPECT_EQ(1, dash::TeamSpec<2>().rank());
  }
  if (num_units >= 4) {
    EXPECT_EQ(num_units, dash::TeamSpec<4>().size());
    EXPECT_EQ(num_units, dash::TeamSpec<4>().extent(0));
    EXPECT_EQ(1, dash::TeamSpec<4>().extent(1));
    EXPECT_EQ(1, dash::TeamSpec<4>().extent(2));
    EXPECT_EQ(1, dash::TeamSpec<4>().extent(3));
    EXPECT_EQ(1, dash::TeamSpec<4>().rank());
  }
}

TEST_F(TeamSpecTest, TeamAndDistributionConstrutor) {
  int num_units = dash::Team::All().size();
  // Team distributed in second dimension (y):
  dash::DistributionSpec<3> dist_blocked_y(
    dash::NONE, dash::BLOCKED, dash::NONE);
  dash::TeamSpec<3> ts_blocked_y(dist_blocked_y, dash::Team::All());
  EXPECT_EQ(num_units, ts_blocked_y.size());
  EXPECT_EQ(1, ts_blocked_y.extent(0));
  EXPECT_EQ(num_units, ts_blocked_y.extent(1));
  EXPECT_EQ(1, ts_blocked_y.extent(2));

  // Team distributed in third dimension (z):
  dash::DistributionSpec<3> dist_blocked_z(
    dash::NONE, dash::NONE, dash::BLOCKED);
  dash::TeamSpec<3> ts_blocked_z(dist_blocked_z, dash::Team::All());
  EXPECT_EQ(num_units, ts_blocked_z.size());
  EXPECT_EQ(1, ts_blocked_z.extent(0));
  EXPECT_EQ(1, ts_blocked_z.extent(1));
  EXPECT_EQ(num_units, ts_blocked_z.extent(2));
}

TEST_F(TeamSpecTest, CopyAndAssignemnt) {
}

