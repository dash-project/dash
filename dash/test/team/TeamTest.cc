
#include "TeamTest.h"

#include <dash/Team.h>
#include <dash/Array.h>
#include <dash/Distribution.h>
#include <dash/Dimensional.h>
#include <dash/util/TeamLocality.h>

#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>


TEST_F(TeamTest, Deallocate) {
  LOG_MESSAGE("Start dealloc test");
  dash::Team & team = dash::Team::All();
  std::stringstream ss;
  ss << team;
  // Test team deallocation
  // Allocate array in scope
  {
    dash::Array<int> array_local(
      10 * dash::size(),
      dash::DistributionSpec<1>(dash::BLOCKED),
      team);
    LOG_MESSAGE("Array allocated, freeing team %s", ss.str().c_str());
    team.free();

    LOG_MESSAGE("Array going out of scope");
  }
  // Array will be deallocated when going out of scope
}

TEST_F(TeamTest, SplitTeamSync)
{
  auto & team_all = dash::Team::All();

  // TODO: This test case has portability issues and fails in
  //       distributed test environments and NastyMPI.
  //       Clarify use case and find variant without writing to
  //       file in `pwd`.
  //
  SKIP_TEST_MSG("not writing to pwd");

  if (team_all.size() < 2) {
    SKIP_TEST_MSG("requires at least 2 units");
  }
  if (!team_all.is_leaf()) {
    SKIP_TEST_MSG("team is already splitted. Skip test");
  }

  // Check if all units are on the same node
  dash::util::TeamLocality tloc(dash::Team::All());
  if(tloc.num_nodes() > 1){
    SKIP_TEST_MSG("test supports only 1 node");
  }

  LOG_MESSAGE("team_all contains %lu units", team_all.size());

  auto & team_core = team_all.split(2);
  LOG_MESSAGE("team_core (%d) contains %lu units",
              team_core.dart_id(), team_core.size());

  if (team_core.num_siblings() < 2) {
    SKIP_TEST_MSG("Team::All().split(2) resulted in < 2 groups");
  }

  ASSERT_EQ_U(team_all, dash::Team::All());

  if (team_core.dart_id() == 1) {
    LOG_MESSAGE("Unit %d: I am in team %d",
                team_core.myid().id, team_core.dart_id());

    usleep(1000 * 1000);
    if (team_core.myid() == 0) {
      std::ofstream outfile ("test.txt");
    }
  }
  LOG_MESSAGE("team_all.myid(): %d, team_core.myid(): %d, dash::myid(): %d",
               team_all.myid().id,  team_core.myid().id,  dash::myid().id);
  LOG_MESSAGE("team_all.position(): %lu, team_core.position(): %lu",
               team_all.position(),     team_core.position());
  LOG_MESSAGE("team_all.dart_id():  %d, team_core.dart_id(): %d",
               team_all.dart_id(),      team_core.dart_id());

  team_all.barrier();

  if (team_core.position() == 0) {
    LOG_MESSAGE("Unit %d: I am in team %d",
                team_core.myid().id, team_core.dart_id());

    if (team_core.myid() == 0) {
      std::ifstream infile("test.txt");
      bool file_exists = infile.good();
      ASSERT_EQ_U(file_exists, true);
    }
  }

  team_all.barrier();

  if (team_all.myid() == 0) {
    std::remove("test.txt");
  }
}

TEST_F(TeamTest, Clone)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("Test requires at least 2 units");
  }

  auto & team_all = dash::Team::All();

  auto & team_core = team_all.split(2);

  ASSERT_EQ_U(team_all, team_core.parent());

  auto *team_to_clone = &team_core;
  for (int i = 0; i < 10; ++i) {
    auto & team_clone = team_to_clone->clone();

    ASSERT_EQ_U(team_all, team_clone.parent());
    ASSERT_EQ_U(i+1, team_clone.num_siblings());
    ASSERT_EQ_U(i+1, team_clone.position());
    ASSERT_EQ_U(team_core.size(), team_clone.size());
    ASSERT_NE_U(team_core.dart_id(), team_clone.dart_id());
    ASSERT_NE_U(team_core.dart_id(), team_clone.dart_id());

    team_to_clone = &team_clone;
  }
}

