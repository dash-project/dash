#include <libdash.h>
#include "TestBase.h"
#include "PatternTest.h"

TEST_F(PatternTest, SimpleConstructor) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 21;
  int extent_y = 37;
  int extent_z = 41;
  int size = extent_x * extent_y * extent_z;
  // Should default to distribution BLOCKED, NONE, NONE:
  dash::Pattern<3> pat_default(extent_x, extent_y, extent_z);
  EXPECT_EQ(dash::DistributionSpec<3>(), pat_default.distspec());
  EXPECT_EQ(dash::Team::All(), pat_default.team());
  EXPECT_EQ(dash::Team::All().size(), pat_default.num_units());
  EXPECT_EQ(size, pat_default.capacity());

  dash::DistributionSpec<3> ds_blocked_z(
      dash::NONE, dash::NONE, dash::BLOCKED);
  dash::Pattern<3> pat_ds(
      extent_x, extent_y, extent_z, 
      ds_blocked_z);
  EXPECT_EQ(ds_blocked_z, pat_ds.distspec());
  EXPECT_EQ(size, pat_ds.capacity());

  // Splits in consecutive test cases within a single test
  // run are not supported for now:
  // dash::Team & team_split_2 = dash::Team::All().split(2);
  dash::Pattern<3> pat_ds_t(
      extent_x, extent_y, extent_z, 
      ds_blocked_z,
      dash::Team::All());
  EXPECT_EQ(ds_blocked_z, pat_ds_t.distspec());
  EXPECT_EQ(size, pat_ds_t.capacity());
  EXPECT_EQ(dash::Team::All().size(), pat_ds_t.num_units());
}

TEST_F(PatternTest, EqualityComparison) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 21;
  int extent_y = 37;
  int extent_z = 41;
  dash::Pattern<3> pat_default(extent_x, extent_y, extent_z);
  EXPECT_EQ(pat_default, pat_default);
}

TEST_F(PatternTest, CopyConstructorAndAssignment) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 12;
  int extent_y = 13;
  int extent_z = 14;
  // Splits in consecutive test cases within a single test
  // run are not supported for now:
  // dash::Team & team_split_2 = dash::Team::All().split(2);
  int num_units = dash::Team::All().size();
  dash::TeamSpec<3> teamspec_2_by_n(2, 1, num_units / 2);
  dash::Pattern<3> pat_org(
      dash::SizeSpec<3>(3, 7, 13),
      dash::DistributionSpec<3>(dash::BLOCKED, dash::NONE, dash::CYCLIC),
      teamspec_2_by_n,
      dash::Team::All());

  dash::Pattern<3> pat_copy(pat_org);
  dash::Pattern<3> pat_assign(extent_x, extent_y, extent_z);
  pat_assign = pat_org;
}

TEST_F(PatternTest, Distribute1DimBlocked) {
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional blocked partitioning:
  //
  // [ .. team 0 .. | .. team 1 .. | ... | team n-1 ]
  size_t team_size  = dash::Team::All().size();
  // Ceil division
  size_t block_size = (_num_elem % team_size == 0)
                      ? _num_elem / team_size
                      : _num_elem / team_size + 1;
  size_t local_cap  = block_size;
  dash::Pattern<1> pat_blocked(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_blocked.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked.blocksize(0), block_size);
  EXPECT_EQ(pat_blocked.max_elem_per_unit(), local_cap);
  
  size_t expected_unit_id = 0;
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    expected_unit_id   = x / block_size;
    expected_coords[0] = x;
    EXPECT_EQ(
      expected_coords,
      pat_blocked.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked.index_to_unit(std::array<long long, 1> { x }));
  }
}

TEST_F(PatternTest, Distribute2DimBlockedY) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in second dimension:
  // Row major:
  // [ team 0[0] | team 0[1] | ... | team 0[2] ]
  // [ team 0[3] | team 0[4] | ... | team 0[5] ]
  // [ team 1[0] | team 1[1] | ... | team 1[2] ]
  // [ team 1[3] | team 1[4] | ... | team 1[5] ]
  // [                   ...                   ]
  // Column major:
  // [ team 0[0] | team 0[2] | ... | team 0[4] ]
  // [ team 0[1] | team 0[3] | ... | team 0[5] ]
  // [ team 1[0] | team 1[2] | ... | team 1[4] ]
  // [ team 1[1] | team 1[3] | ... | team 1[5] ]
  // [                   ...                   ]
  int team_size = dash::Team::All().size();
  int extent_x  = 5;
  int extent_y  = 4;
  size_t size   = extent_x * extent_y;
  // Ceil division
  int block_size_x = extent_x;
  int block_size_y = (extent_y % team_size == 0)
                       ? extent_y / team_size
                       : extent_y / team_size + 1;
  int max_per_unit = block_size_x * block_size_y;
  dash::Pattern<2, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), size);
  EXPECT_EQ(pat_blocked_row.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  LOG_MESSAGE("block size: x: %d, y: %d",
    block_size_x, block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order =
        expected_index_row_order % max_per_unit;
      int expected_offset_col_order =
        y % block_size_y + x * block_size_y;
      int expected_unit_id = y / block_size_y;

      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_row_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }));

      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }));
    }
  }
}

TEST_F(PatternTest, Distribute2DimBlockedX) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ team 0[0] | team 1[0] | team 2[0] | ... | team n-1 ]
  // [ team 0[1] | team 1[1] | team 2[1] | ... | team n-1 ]
  // [ team 0[2] | team 1[2] | team 2[2] | ... | team n-1 ]
  // [ team 0[3] | team 1[3] | team 2[3] | ... | team n-1 ]
  // [                       ...                          ]
  int team_size    = dash::Team::All().size();
  int extent_x     = 4;
  int extent_y     = 5;
  size_t size      = extent_x * extent_y;
  // Ceil division
  int block_size_x = (extent_x % team_size == 0)
                       ? extent_x / team_size
                       : extent_x / team_size + 1;
  int block_size_y = extent_y;
  int max_per_unit = block_size_x * block_size_y;
  dash::Pattern<2, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), size);
  EXPECT_EQ(pat_blocked_row.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order = (x % block_size_x) + y * block_size_x;
      int expected_offset_col_order = expected_index_col_order % 
                                        max_per_unit;
      int expected_unit_id = x / block_size_x;

      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_row_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }));

      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }));
    }
  }
}

TEST_F(PatternTest, Distribute1DimCyclic) {
  DASH_TEST_LOCAL_ONLY();
}

TEST_F(PatternTest, Distribute1DimBlockcyclic) {
  DASH_TEST_LOCAL_ONLY();
}
