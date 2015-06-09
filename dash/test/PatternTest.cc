#include <libdash.h>
#include "TestBase.h"
#include "PatternTest.h"

TEST_F(PatternTest, Distribute1DimBlocked) {
  DASH_TEST_LOCAL_ONLY();
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
  
  int expected_unit_id = 0;
  for (int x = 0; x < _num_elem; ++x) {
    expected_unit_id = x / block_size;
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked.index_to_unit(std::array<long long, 1> { x }));
  }
}

TEST_F(PatternTest, Distribute2DimBlockedY) {
  return;
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in second dimension:
  // 
  // [ .. team 0 ..   | .. team 0 ..   | ... | team 0   ]
  // [ .. team 0 ..   | .. team 0 ..   | ... | team 0   ]
  // [ .. team 1 ..   | .. team 1 ..   | ... | team 1   ]
  // [ .. team 1 ..   | .. team 1 ..   | ... | team 1   ]
  // [                       ...                        ]
  // [ .. team n-1 .. | .. team n-1 .. | ... | team n-1 ]
  int team_size = dash::Team::All().size();
  int extent_x  = 3;
  int extent_y  = 3;
  size_t size   = extent_x * extent_y;
  // Ceil division
  int block_size_x = extent_x;
  int block_size_y = (extent_y % team_size == 0)
                       ? extent_y / team_size
                       : extent_y / team_size + 1;
  int max_per_unit = block_size_x * block_size_y;
  dash::Pattern<2> pat_blocked(
      dash::SizeSpec<2, dash::ROW_MAJOR>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked.capacity(), size);
  EXPECT_EQ(pat_blocked.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked.blocksize(1), block_size_y);
  int expected_unit_id = 0;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order = (y * extent_x) + x;
      int expected_index_col_order = (x * extent_y) + y;
      int expected_offset_row_order = expected_index_row_order % max_per_unit;
      int expected_offset_col_order = expected_index_col_order % max_per_unit;
      expected_unit_id = y / block_size_y;
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked.index_to_elem(std::array<long long, 2> { x, y }));
    }
  }
}

TEST_F(PatternTest, Distribute2DimBlockedX) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ .. team 0 .. | .. team 1 .. | .. team 2 .. | ... | .. team n-1 .. ]
  // [ .. team 0 .. | .. team 1 .. | .. team 2 .. | ... | .. team n-1 .. ]
  // [ .. team 0 .. | .. team 1 .. | .. team 2 .. | ... | .. team n-1 .. ]
  // [ .. team 0 .. | .. team 1 .. | .. team 2 .. | ... | .. team n-1 .. ]
  // [                               ...                                 ]
  int team_size    = dash::Team::All().size();
  int extent_x     = 3;
  int extent_y     = 3;
  size_t size      = extent_x * extent_y;
  // Ceil division
  int block_size_x = (extent_x % team_size == 0)
                       ? extent_x / team_size
                       : extent_x / team_size + 1;
  int block_size_y = extent_y;
  int max_per_unit = block_size_x * block_size_y;
  dash::Pattern<2> pat_blocked(
      dash::SizeSpec<2, dash::ROW_MAJOR>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked.capacity(), size);
  EXPECT_EQ(pat_blocked.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked.blocksize(1), block_size_y);
  int expected_unit_id = 0;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order = expected_index_row_order % max_per_unit;
      int expected_offset_col_order = expected_index_col_order % max_per_unit;
      expected_unit_id = x / block_size_x;
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked.index_to_elem(std::array<long long, 2> { x, y }));
    }
  }
}

TEST_F(PatternTest, Distribute1DimCyclic) {
  DASH_TEST_LOCAL_ONLY();
}

TEST_F(PatternTest, Distribute1DimBlockcyclic) {
  DASH_TEST_LOCAL_ONLY();
}
