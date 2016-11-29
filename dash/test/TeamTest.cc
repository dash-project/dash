#include <dash/Array.h>
#include <dash/Init.h>
#include <dash/Team.h>

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

