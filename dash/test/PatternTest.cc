#include <libdash.h>
#include "TestBase.h"
#include "PatternTest.h"

TEST_F(PatternTest, Distribute1DimBlocked) {
  int team_size  = dash::Team::All().size();
  // Ceil division
  int block_size = (_num_elem % team_size == 0)
                     ? _num_elem / team_size
                     : _num_elem / team_size + 1;
  dash::Pattern<1> pat_blocked(
      dash::SizeSpec<1, dash::ROW_MAJOR>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Fails:
  EXPECT_EQ(pat_blocked.capacity(), _num_elem);
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

TEST_F(PatternTest, Distribute1DimCyclic) {
}

TEST_F(PatternTest, Distribute1DimBlockcyclic) {
}
