#include <libdash.h>
#include "TestBase.h"
#include "PatternTest.h"

TEST_F(PatternTest, Distribute1DimBlocked) {
  // Simple 1-dimensional blocked partitioning:
  //
  // [ .. team 0 .. | .. team 1 .. | ... | team n-1 ]
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
  EXPECT_EQ(pat_blocked.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked.blocksize(0), block_size);
  // First element -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> { 0 }),
      0);
  // Last element in first block -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> { block_size-1 }),
      0);
  // First element in second block -> unit 1
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 1> { block_size }),
      1);
}

TEST_F(PatternTest, Distribute2DimBlocked) {
  // 2-dimensional, blocked partitioning in second dimension:
  // 
  // [ .. team 0 ..   | .. team 0 ..   | ... | team 0   ]
  // [ .. team 0 ..   | .. team 0 ..   | ... | team 0   ]
  // [ .. team 1 ..   | .. team 1 ..   | ... | team 1   ]
  // [ .. team 1 ..   | .. team 1 ..   | ... | team 1   ]
  // [                       ...                        ]
  // [ .. team n-1 .. | .. team n-1 .. | ... | team n-1 ]
  int team_size   = dash::Team::All().size();
  int extent_x    = 100;
  int extent_y    = 20;
  size_t size     = extent_x * extent_y;
  // Ceil division
  int block_size_x = extent_x;
  int block_size_y = (extent_y % team_size == 0)
                       ? extent_y / team_size
                       : extent_y / team_size + 1;
  dash::Pattern<2> pat_blocked(
      dash::SizeSpec<2, dash::ROW_MAJOR>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked.capacity(), size);
  EXPECT_EQ(pat_blocked.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked.blocksize(1), block_size_y);
  // First element -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 2> { 0, 0 }),
      0);
  // Last element in first block -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 2> { block_size_x-1, 0 }),
      0);
  // First element in second block -> unit 0
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 2> { block_size_x, 0 }),
      0);
  // First element in second row -> unit 1
  EXPECT_EQ(
      pat_blocked.index_to_unit(std::array<long long, 2> { 0, block_size_y }),
      1);
}


TEST_F(PatternTest, Distribute1DimCyclic) {
}

TEST_F(PatternTest, Distribute1DimBlockcyclic) {
}
