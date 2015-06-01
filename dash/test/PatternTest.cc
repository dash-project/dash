#include <libdash.h>
#include "PatternTest.h"

TEST_F(PatternTest, DistributeDim1) {
  dash::Pattern<1> pat(
      dash::SizeSpec<1, dash::ROW_MAJOR>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // First element -> first unit
  EXPECT_EQ(pat.atunit(0), 0);
}
