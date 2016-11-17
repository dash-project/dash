#include <libdash.h>
#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>  

#include "TestBase.h"
#include "TeamTest.h"

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
  if(team_all.size() < 2){
    SKIP_TEST();
  }
  LOG_MESSAGE("team_all contains %d units", team_all.size());

  auto & team_core = team_all.locality_split(dash::util::Locality::Scope::Core, 2);
  LOG_MESSAGE("team_core (%d) contains %d units", team_core.dart_id(), team_core.size());

  ASSERT_EQ_U(team_all, dash::Team::All());

  if(team_core.dart_id() == 1){
    LOG_MESSAGE("Unit %d: I am in team %d", team_core.myid(), team_core.dart_id());
    usleep(1000 * 1000);
    if(team_core.myid() == 0){
      std::ofstream outfile ("test.txt");
    }  
  }
  LOG_MESSAGE("team_all.myid(): %d, team_core.myid(): %d, dash::myid(): %d",
               team_all.myid(),     team_core.myid(),     dash::myid());
  team_all.barrier();
  if(team_core.dart_id() == 2){
    LOG_MESSAGE("Unit %d: I am in team %d", team_core.myid(), team_core.dart_id());
    if(team_core.myid() == 0){
      std::ifstream infile("test.txt");
      bool file_exists = infile.good();
      ASSERT_EQ_U(file_exists, true);
    }
  }
  team_all.barrier();
  if(team_all.myid() == 0){
    std::remove("test.txt");
  }
}
