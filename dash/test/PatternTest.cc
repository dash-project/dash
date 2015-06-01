#include <libdash.h>
#include "PatternTest.h"

TEST_F(PatternTest, DistributeDim1) {
  int team_size  = dash::Team::All().size();
  int block_size = ceil(_num_elem / team_size);
  dash::Pattern<1> pat_blocked(
      dash::SizeSpec<1, dash::ROW_MAJOR>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Fails:
  EXPECT_EQ(pat_blocked.nelem(), _num_elem);
  EXPECT_EQ(pat_blocked.blocksize(1), block_size);
  // First element -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> {0}),
      0);
  // Last element in first block -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> {block_size-1}),
      0);
  // First element in second block -> unit 1
  // Fails:
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> {block_size}),
      1);
}
