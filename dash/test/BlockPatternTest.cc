#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "BlockPatternTest.h"

TEST_F(BlockPatternTest, SimpleConstructor)
{
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
  dash::Pattern<3, dash::COL_MAJOR> pat_ds(
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

TEST_F(BlockPatternTest, EqualityComparison)
{
  DASH_TEST_LOCAL_ONLY();
  int num_units = dash::Team::All().size();
  int extent_x = 21;
  int extent_y = 37;
  int extent_z = 41;
  dash::Pattern<3> pat_1(extent_x, extent_y, extent_z);
  dash::Pattern<3> pat_2(extent_x, extent_y + 1, extent_z);
  dash::Pattern<3> pat_3(extent_x, extent_y + 1, extent_z,
                         dash::NONE, dash::BLOCKED, dash::NONE);
  dash::Pattern<3> pat_4(extent_x, extent_y + 1, extent_z,
                         dash::TeamSpec<3>(1,num_units,1));
  dash::Pattern<3> pat_5(extent_x, extent_y, extent_z,
                         dash::TeamSpec<3>(num_units,1,1));
  EXPECT_EQ(pat_1, pat_1);
  EXPECT_EQ(pat_1, pat_5);
  EXPECT_NE(pat_1, pat_2);
  EXPECT_NE(pat_1, pat_3);
  EXPECT_NE(pat_1, pat_4);
}

TEST_F(BlockPatternTest, CopyConstructorAndAssignment)
{
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 12;
  int extent_y = 13;
  int extent_z = 14;
  // Splits in consecutive test cases within a single test
  // run are not supported for now:
  // dash::Team & team_split_2 = dash::Team::All().split(2);
  int num_units = dash::Team::All().size();
  if (num_units % 2 == 0) {
    // This test requires that (2 * 1 * (num_units/2)) == num_units
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
}

TEST_F(BlockPatternTest, Distribute1DimBlocked)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;

  // Simple 1-dimensional blocked partitioning:
  //
  // [ .. team 0 .. | .. team 1 .. | ... | team n-1 ]
  size_t team_size  = dash::Team::All().size();
  LOG_MESSAGE("Team size: %lu", team_size);
  // One underfilled block:
  _num_elem = 11 * team_size - 1;
  size_t block_size = dash::math::div_ceil(_num_elem, team_size);
  size_t local_cap  = block_size;
  dash::Pattern<1, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  dash::Pattern<1, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size);
  EXPECT_EQ(pat_blocked_row.local_capacity(), local_cap);
  EXPECT_EQ(pat_blocked_col.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size);
  EXPECT_EQ(pat_blocked_col.local_capacity(), local_cap);
  // Test local extents:
  for (dash::team_unit_t u{0}; u < team_size; ++u) {
    size_t local_extent_x;
    if (u < _num_elem / block_size) {
      // Full block
      local_extent_x = block_size;
    } else if (u == _num_elem / block_size) {
      // Underfilled block
      local_extent_x = _num_elem % block_size;
    } else {
      // Empty block
      local_extent_x = 0;
    }
    LOG_MESSAGE("local extents: u:%lu, le:%lu",
      u.id, local_extent_x);
    EXPECT_EQ(local_extent_x, pat_blocked_row.local_extents(u)[0]);
    EXPECT_EQ(local_extent_x, pat_blocked_col.local_extents(u)[0]);
  }
  std::array<index_t, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    dash::team_unit_t expected_unit_id(x / block_size);
    int expected_offset  = x % block_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_row.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_row.at(std::array<index_t, 1> { x }));
    auto glob_coords_row =
      pat_blocked_row.global(
        expected_unit_id, std::array<index_t, 1> { expected_offset });
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      glob_coords_row);
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_col.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_col.at(std::array<index_t, 1> { x }));
    auto glob_coords_col =
      pat_blocked_col.global(
        expected_unit_id, std::array<index_t, 1> { expected_offset });
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      glob_coords_col);
  }
}

TEST_F(BlockPatternTest, Distribute1DimCyclic)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;
  // Simple 1-dimensional cyclic partitioning:
  //
  // [ team 0 | team 1 | team 0 | team 1 | ... ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = dash::math::div_ceil(_num_elem, team_size);
  size_t local_cap  = block_size;
  dash::Pattern<1, dash::ROW_MAJOR> pat_cyclic_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::CYCLIC),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Column order must be irrelevant:
  dash::Pattern<1, dash::COL_MAJOR> pat_cyclic_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::CYCLIC),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_cyclic_row.capacity(), _num_elem);
  EXPECT_EQ(pat_cyclic_row.blocksize(0), 1);
  EXPECT_EQ(pat_cyclic_row.local_capacity(), local_cap);
  EXPECT_EQ(pat_cyclic_col.capacity(), _num_elem);
  EXPECT_EQ(pat_cyclic_col.blocksize(0), 1);
  EXPECT_EQ(pat_cyclic_col.local_capacity(), local_cap);
  std::array<index_t, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    dash::team_unit_t expected_unit_id(x % team_size);
    int expected_offset  = x / team_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_row.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_row.at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      pat_cyclic_row.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_col.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_col.at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      pat_cyclic_col.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
  }
}

TEST_F(BlockPatternTest, Distribute1DimBlockcyclic)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;
  // Simple 1-dimensional blocked partitioning:
  //
  // [ team 0 | team 1 | team 0 | team 1 | ... ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = 23;
  size_t num_blocks = dash::math::div_ceil(_num_elem, block_size);
  size_t local_cap  = block_size *
                        dash::math::div_ceil(num_blocks, team_size);
  dash::Pattern<1, dash::ROW_MAJOR> pat_blockcyclic_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKCYCLIC(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Column order must be irrelevant:
  dash::Pattern<1, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKCYCLIC(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_blockcyclic_row.capacity(), _num_elem);
  EXPECT_EQ(pat_blockcyclic_row.blocksize(0), block_size);
  EXPECT_EQ(pat_blockcyclic_row.local_capacity(), local_cap);
  EXPECT_EQ(pat_blockcyclic_col.capacity(), _num_elem);
  EXPECT_EQ(pat_blockcyclic_col.blocksize(0), block_size);
  EXPECT_EQ(pat_blockcyclic_col.local_capacity(), local_cap);
  LOG_MESSAGE("num elem: %d, block size: %lu, num blocks: %lu",
    _num_elem, block_size, num_blocks);
  std::array<index_t, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    dash::team_unit_t unit_id((x / block_size) % team_size);
    int block_index       = x / block_size;
    int block_base_offset = block_size * (block_index / team_size);
    dash::team_unit_t expected_unit_id(block_index % team_size);
    int expected_offset   = (x % block_size) + block_base_offset;
    int expected_index    = x;
    expected_coords[0]    = x;
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_row.coords(x));
    EXPECT_TRUE(
      pat_blockcyclic_row.is_local(x, unit_id));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_row.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_row.at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      pat_blockcyclic_row.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_col.coords(x));
    EXPECT_TRUE(
      pat_blockcyclic_col.is_local(x, unit_id));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_col.unit_at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_col.at(std::array<index_t, 1> { x }));
    EXPECT_EQ(
      (std::array<index_t, 1> { expected_index }),
      pat_blockcyclic_col.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
  }
}

TEST_F(BlockPatternTest, Distribute2DimBlockedY)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t  index_t;

  typedef dash::Pattern<2, dash::ROW_MAJOR>
    pattern_rowmajor_t;
  typedef dash::Pattern<2, dash::COL_MAJOR>
    pattern_colmajor_t;

  // 2-dimensional, blocked partitioning in first dimension:
  // Row major:
  // [ unit 0[0] | unit 0[1] | ... | unit 0[2] ]
  // [ unit 0[3] | unit 0[4] | ... | unit 0[5] ]
  // [ unit 1[0] | unit 1[1] | ... | unit 1[2] ]
  // [ unit 1[3] | unit 1[4] | ... | unit 1[5] ]
  // [                   ...                   ]
  // Column major:
  // [ unit 0[0] | unit 0[2] | ... | unit 0[4] ]
  // [ unit 0[1] | unit 0[3] | ... | unit 0[5] ]
  // [ unit 1[0] | unit 1[2] | ... | unit 1[4] ]
  // [ unit 1[1] | unit 1[3] | ... | unit 1[5] ]
  // [                   ...                   ]
  size_t team_size      = dash::Team::All().size();
  size_t extent_x       = 17;
  size_t extent_y       = 5 + team_size * 3;
  size_t size           = extent_x * extent_y;
  // Ceil division
  size_t block_size_x   = extent_x;
  size_t block_size_y   = dash::math::div_ceil(extent_y, team_size);
  size_t max_per_unit   = block_size_x * block_size_y;
  size_t overflow_bs_x  = extent_x % block_size_x;
  size_t overflow_bs_y  = extent_y % block_size_y;
  size_t underfill_bs_x = (overflow_bs_x == 0)
                          ? 0
                          : block_size_x - overflow_bs_x;
  size_t underfill_bs_y = (overflow_bs_y == 0)
                          ? 0
                          : block_size_y - overflow_bs_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpu: %d",
      extent_y, extent_x,
      block_size_y, block_size_x,
      max_per_unit);
  pattern_rowmajor_t pat_blocked_row(
      dash::SizeSpec<2>(extent_y, extent_x),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  pattern_colmajor_t pat_blocked_col(
      dash::SizeSpec<2>(extent_y, extent_x),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());

  dash::test::print_pattern_mapping(
    "pattern.rowmajor.unit_at", pat_blocked_row, 1,
    [](const pattern_rowmajor_t & _pattern, int _x, int _y) -> dart_unit_t {
        return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
    });
  dash::test::print_pattern_mapping(
    "pattern.rowmajor.local_index", pat_blocked_row, 3,
    [](const pattern_rowmajor_t & _pattern, int _x, int _y) -> index_t {
        return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
    });
  dash::test::print_pattern_mapping(
    "pattern.colmajor.unit_at", pat_blocked_col, 1,
    [](const pattern_colmajor_t & _pattern, int _x, int _y) -> dart_unit_t {
        return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
    });
  dash::test::print_pattern_mapping(
    "pattern.colmajor.local_index", pat_blocked_col, 3,
    [](const pattern_colmajor_t & _pattern, int _x, int _y) -> index_t {
        return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
    });

  EXPECT_EQ(pat_blocked_row.capacity(), size);
  EXPECT_EQ(pat_blocked_row.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_y);
  EXPECT_EQ(pat_blocked_row.underfilled_blocksize(1), underfill_bs_x);
  EXPECT_EQ(pat_blocked_row.underfilled_blocksize(0), underfill_bs_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_y);
  EXPECT_EQ(pat_blocked_col.underfilled_blocksize(1), underfill_bs_x);
  EXPECT_EQ(pat_blocked_col.underfilled_blocksize(0), underfill_bs_y);
  LOG_MESSAGE("block size: x: %d, y: %d",
    block_size_x, block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      // Units might have empty local range, e.g. when distributing 41
      // elements to 8 units.
      int num_blocks_y              = dash::math::div_ceil(
                                        extent_y, block_size_y);
      // Subtract missing elements in last block if any:
      int underfill_y               = (y >= (num_blocks_y-1) *
                                            block_size_y)
                                      ? (block_size_y * num_blocks_y) -
                                        extent_y
                                      : 0;
      // Actual extent of block, adjusted for underfilled extent:
      int block_size_y_adj          = block_size_y - underfill_y;
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order = expected_index_row_order % max_per_unit;
      int expected_offset_col_order = (y % block_size_y) + (x *
                                        block_size_y_adj);
      dash::team_unit_t expected_unit_id(y / block_size_y);
      int local_x                   = x;
      int local_y                   = y % block_size_y;
      // Row major:
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.unit_at(std::array<index_t, 2> { y, x }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.at(std::array<index_t, 2> { y, x }));
      EXPECT_EQ(
        (std::array<index_t, 2> { y, x }),
        pat_blocked_row.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_y, local_x  })));
      // Col major:
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.unit_at(std::array<index_t, 2> { y, x }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.at(std::array<index_t, 2> { y, x }));
      EXPECT_EQ(
        (std::array<index_t, 2> { y, x }),
        pat_blocked_col.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_y, local_x })));
    }
  }
}

TEST_F(BlockPatternTest, Distribute2DimBlockedX)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;
  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 1[0] | team 2[0] | ... | team n-1 ]
  // [ team 0[1] | team 1[1] | team 2[1] | ... | team n-1 ]
  // [ team 0[2] | team 1[2] | team 2[2] | ... | team n-1 ]
  // [ team 0[3] | team 1[3] | team 2[3] | ... | team n-1 ]
  // [                       ...                          ]
  int team_size    = dash::Team::All().size();
  int extent_x     = 41;
  int extent_y     = 17;
  size_t size      = extent_x * extent_y;
  // Ceil division
  int block_size_x = dash::math::div_ceil(extent_x, team_size);
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
  EXPECT_EQ(pat_blocked_row.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      // Units might have empty local range, e.g. when distributing 41
      // elements to 8 units.
      int num_blocks_x              = dash::math::div_ceil(
                                        extent_x, block_size_x);
      // Subtract missing elements in last block if any:
      int underfill_x               = (x >= (num_blocks_x-1) *
                                        block_size_x)
                                      ? (block_size_x * num_blocks_x) -
                                        extent_x
                                      : 0;
      // Actual extent of block, adjusted for underfilled extent:
      int block_size_x_adj          = block_size_x - underfill_x;
      int expected_index_col_order  = (y * extent_x) + x;
      int expected_index_row_order  = (x * extent_y) + y;
      int expected_offset_col_order = (x % block_size_x) +
                                      (y * block_size_x_adj);
      int expected_offset_row_order = expected_index_row_order %
                                        max_per_unit;
      dash::team_unit_t expected_unit_id(x / block_size_x);
      int local_x                   = x % block_size_x;
      int local_y                   = y;
      // Row major:
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.unit_at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        pat_blocked_row.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_x, local_y })));
      // Col major:
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.unit_at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        pat_blocked_col.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_x, local_y })));
    }
  }
}

TEST_F(BlockPatternTest, Distribute2DimBlockcyclicXY)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t  index_t;
  typedef std::array<index_t, 2> coords_t;
  // 2-dimensional, blocked partitioning in two dimensions:
  //
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] ]
  // [ team 2[0] | team 3[0] | team 2[1] | team 3[1] ]
  // [ team 2[2] | team 3[2] | team 2[3] | team 3[3] ]
  // [                      ...                      ]
  int team_size    = dash::Team::All().size();
  if (team_size < 4) {
    LOG_MESSAGE("Skipping test Distribute2DimBlockcyclicXY, "
                "at least 4 units nedded");
    return;
  }
  if (team_size % 2 != 0) {
    LOG_MESSAGE("Skipping test Distribute2DimBlockcyclicXY, "
                "number of units must be multiple of 2");
    return;
  }
  size_t extent_x     = 5 + team_size;
  size_t extent_y     = 3 + team_size;
  size_t size         = extent_x * extent_y;
  // Ceil division
  size_t block_size_x = 3;
  size_t block_size_y = 2;
  size_t num_units_x  = team_size / 2;
  size_t num_units_y  = 2;
  size_t block_size   = block_size_x * block_size_y;
  size_t num_blocks_x = dash::math::div_ceil(extent_x, block_size_x);
  size_t num_blocks_y = dash::math::div_ceil(extent_y, block_size_y);
  size_t max_per_unit = dash::math::div_ceil(num_blocks_x, num_units_x) *
                        dash::math::div_ceil(num_blocks_y, num_units_y) *
                        block_size;
  dash::TeamSpec<2> ts(num_units_x, num_units_y);
  EXPECT_EQ_U(ts.size(), team_size);
  EXPECT_EQ_U(ts.rank(), 2);
  dash::Pattern<2, dash::ROW_MAJOR> pat_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::BLOCKCYCLIC(block_size_x),
        dash::BLOCKCYCLIC(block_size_y)),
      dash::TeamSpec<2>(num_units_x, num_units_y),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::BLOCKCYCLIC(block_size_x),
        dash::BLOCKCYCLIC(block_size_y)),
      dash::TeamSpec<2>(num_units_x, num_units_y),
      dash::Team::All());

  typedef decltype(pat_row) pattern_rmaj_t;
  typedef decltype(pat_col) pattern_cmaj_t;

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.row-major.local_index", pat_row, 6,
      [](const pattern_rmaj_t & _pattern, int _x, int _y) -> std::string {
          auto lpos    = _pattern.local_index(coords_t {_x, _y});
          auto unit    = lpos.unit;
          auto lindex  = lpos.index;
          std::ostringstream ss;
          ss << "u" << unit << "("
             << std::setw(2) << lindex
             << ")";
          return ss.str();
      });
    dash::test::print_pattern_mapping(
      "pattern.col-major.local_index", pat_col, 6,
      [](const pattern_cmaj_t & _pattern, int _x, int _y) -> std::string {
          auto lpos    = _pattern.local_index(coords_t {_x, _y});
          auto unit    = lpos.unit;
          auto lindex  = lpos.index;
          std::ostringstream ss;
          ss << "u" << unit << "("
             << std::setw(2) << lindex
             << ")";
          return ss.str();
      });
  }

  EXPECT_EQ_U(pat_row.team().size(), team_size);
  EXPECT_EQ_U(pat_row.teamspec().size(), team_size);
  EXPECT_EQ_U(pat_row.capacity(), size);
  EXPECT_EQ_U(pat_row.local_capacity(), max_per_unit);
  EXPECT_EQ_U(pat_row.blocksize(0), block_size_x);
  EXPECT_EQ_U(pat_row.blocksize(1), block_size_y);
  EXPECT_EQ_U(pat_col.team().size(), team_size);
  EXPECT_EQ_U(pat_col.teamspec().size(), team_size);
  EXPECT_EQ_U(pat_col.capacity(), size);
  EXPECT_EQ_U(pat_col.local_capacity(), max_per_unit);
  EXPECT_EQ_U(pat_col.blocksize(0), block_size_x);
  EXPECT_EQ_U(pat_col.blocksize(1), block_size_y);
  for (int x = 0; x < static_cast<int>(extent_x); ++x) {
    for (int y = 0; y < static_cast<int>(extent_y); ++y) {
      // Units might have empty local range, e.g. when distributing 41
      // elements to 8 units.
      // Subtract missing elements in last block if any:
      int underfill_x               = (x >= static_cast<int>(
                                             (num_blocks_x-1) * block_size_x))
                                      ? (block_size_x * num_blocks_x) -
                                        extent_x
                                      : 0;
      // Actual extent of block, adjusted for underfilled extent:
      int block_size_x_adj          = block_size_x - underfill_x;
      int expected_offset_row_order = 0;
      int expected_offset_col_order = 0;
      int block_coord_x             = (x / block_size_x) % num_units_x;
      int block_coord_y             = (y / block_size_y) % num_units_y;
      int expected_unit_id          = block_coord_x * num_units_y +
                                      block_coord_y;
//    int local_x                   = x % block_size_x;
//    int local_y                   = y;
      // Row major:
      EXPECT_EQ(
        expected_unit_id,
        pat_row.unit_at(std::array<index_t, 2> { x, y }));
#if DEACT
      EXPECT_EQ(
        expected_offset_row_order,
        pat_row.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        pat_row.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_x, local_y })));
#endif
      // Col major:
      EXPECT_EQ(
        expected_unit_id,
        pat_col.unit_at(std::array<index_t, 2> { x, y }));
#if DEACT
      EXPECT_EQ(
        expected_offset_col_order,
        pat_col.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        pat_col.global(
          expected_unit_id,
          (std::array<index_t, 2> { local_x, local_y })));
#endif
    }
  }
}

TEST_F(BlockPatternTest, Distribute2DimCyclicX)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;
  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] | ... ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] | ... ]
  // [ team 0[4] | team 1[4] | team 0[5] | team 1[5] | ... ]
  // [ team 0[6] | team 1[6] | team 0[7] | team 1[7] | ... ]
  // [                        ...                          ]
  int team_size      = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  int extent_x       = team_size + 7;
  int extent_y       = 23;
  size_t size        = extent_x * extent_y;
  int block_size_x   = 1;
  int max_per_unit_x = dash::math::div_ceil(extent_x, team_size);
  int block_size_y   = extent_y;
  int max_per_unit   = max_per_unit_x * block_size_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpx: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit_x, max_per_unit);
  dash::Pattern<2, dash::ROW_MAJOR> pat_cyclic_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_cyclic_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_cyclic_row.capacity(), size);
  EXPECT_EQ(pat_cyclic_row.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_cyclic_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_cyclic_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_cyclic_col.capacity(), size);
  EXPECT_EQ(pat_cyclic_col.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_cyclic_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_cyclic_col.blocksize(1), block_size_y);
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
  int num_overflow_blocks = extent_x % team_size;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int unit_id                   = x % team_size;
      int min_blocks_x              = extent_x / team_size;
      int num_add_blocks_x          = extent_x % team_size;
      int num_blocks_unit_x         = min_blocks_x;
      if (unit_id < num_add_blocks_x) {
        num_blocks_unit_x++;
      }
//    int expected_index_row_order  = (y * extent_x) + x;
//    int expected_index_col_order  = (x * extent_y) + y;
      dash::team_unit_t expected_unit_id(x % team_size);
      int expected_offset_col_order = (y * num_blocks_unit_x) +
                                      x / team_size;
      int expected_offset_row_order = ((x / team_size) * extent_y) + y;
      int local_x                   = x / team_size;
      int local_y                   = y;
      auto glob_coords_row =
        pat_cyclic_row.global(
          expected_unit_id,
          std::array<index_t, 2> { local_x, local_y });
      auto glob_coords_col =
        pat_cyclic_col.global(
          expected_unit_id,
          std::array<index_t, 2> { local_x, local_y });
      // Row major:
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_row.unit_at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_cyclic_row.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        glob_coords_row);
      // Col major:
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_col.unit_at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_cyclic_col.at(std::array<index_t, 2> { x, y }));
      EXPECT_EQ(
        (std::array<index_t, 2> { x, y }),
        glob_coords_col);
    }
  }
}

TEST_F(BlockPatternTest, Distribute3DimBlockcyclicX)
{
  DASH_TEST_LOCAL_ONLY();
  typedef dash::default_index_t index_t;
  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] | ... ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] | ... ]
  // [ team 0[4] | team 1[4] | team 0[5] | team 1[5] | ... ]
  // [ team 0[6] | team 1[6] | team 0[7] | team 1[7] | ... ]
  // [                        ...                          ]
  size_t team_size    = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t extent_x     = 19 + team_size;
  size_t extent_y     = 51;
  size_t extent_z     = 3;
  size_t size         = extent_x * extent_y * extent_z;
  size_t block_size_x = 2;
  size_t num_blocks_x    = dash::math::div_ceil(extent_x, block_size_x);
  size_t max_per_unit_x  = block_size_x *
                           dash::math::div_ceil(num_blocks_x, team_size);
  size_t block_size_y    = extent_y;
  size_t block_size_z    = extent_z;
  size_t max_per_unit    = max_per_unit_x * block_size_y * block_size_z;
  LOG_MESSAGE("ex: %lu, ey: %lu, ez: %lu, bsx: %lu, bsy: %lu, "
              "mpx: %lu, mpu: %lu",
              extent_x, extent_y, extent_z,
              block_size_x, block_size_y,
              max_per_unit_x, max_per_unit);
  dash::Pattern<3, dash::ROW_MAJOR> pat_blockcyclic_row(
      dash::SizeSpec<3>(extent_x, extent_y, extent_z),
      dash::DistributionSpec<3>(dash::BLOCKCYCLIC(block_size_x),
                                dash::NONE,
                                dash::NONE),
      dash::TeamSpec<3>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<3, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<3>(extent_x, extent_y, extent_z),
      dash::DistributionSpec<3>(dash::BLOCKCYCLIC(block_size_x),
                                dash::NONE,
                                dash::NONE),
      dash::TeamSpec<3>(dash::Team::All()),
      dash::Team::All());
  ASSERT_EQ(pat_blockcyclic_row.capacity(), size);
  ASSERT_EQ(pat_blockcyclic_row.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_blockcyclic_row.blocksize(0), block_size_x);
  ASSERT_EQ(pat_blockcyclic_row.blocksize(1), block_size_y);
  ASSERT_EQ(pat_blockcyclic_row.blocksize(2), block_size_z);
  ASSERT_EQ(pat_blockcyclic_col.capacity(), size);
  ASSERT_EQ(pat_blockcyclic_col.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_blockcyclic_col.blocksize(0), block_size_x);
  ASSERT_EQ(pat_blockcyclic_col.blocksize(1), block_size_y);
  ASSERT_EQ(pat_blockcyclic_col.blocksize(2), block_size_z);
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
//int num_overflow_blocks = extent_x % team_size;
  for (int x = 0; x < static_cast<int>(extent_x); ++x) {
    for (int y = 0; y < static_cast<int>(extent_y); ++y) {
      for (int z = 0; z < static_cast<int>(extent_z); ++z) {
        dash::team_unit_t unit_id((x / block_size_x) % team_size);
        int num_blocks_x              = dash::math::div_ceil(
                                          extent_x, block_size_x);
        int min_blocks_x              = dash::math::div_ceil(
                                          extent_x, block_size_x) /
                                        team_size;
        int num_add_blocks_x          = num_blocks_x % team_size;
        int overflow_block_size_x     = extent_x % block_size_x;
        int num_blocks_unit_x         = min_blocks_x;
        int num_add_elem_x            = 0;
        int block_offset_x            = x / block_size_x;
        int last_block_unit           = (num_blocks_x % team_size == 0)
                                        ? team_size-1
                                        : (num_blocks_x % team_size) - 1;
        if (unit_id < num_add_blocks_x) {
          num_blocks_unit_x++;
          num_add_elem_x += block_size_x;
        }
        if (unit_id == last_block_unit) {
          if (overflow_block_size_x > 0) {
            num_add_elem_x -= block_size_x - overflow_block_size_x;
          }
        }
        int local_extent_x            = (min_blocks_x * block_size_x) +
                                        num_add_elem_x;
        int local_block_index_x       = block_offset_x / team_size;
        int expected_index_col_order  = (z * extent_y * extent_x) +
                                        (y * extent_x) + x;
        int expected_index_row_order  = (x * extent_y * extent_z) +
                                        (y * extent_z) + z;
        dash::team_unit_t expected_unit_id((x / block_size_x) % team_size);
        int local_index_x             = (local_block_index_x *
                                          block_size_x) +
                                        (x % block_size_x);
        int expected_offset_col_order = local_index_x +
                                        (y * local_extent_x) +
                                        (z * local_extent_x * extent_y);
        int expected_offset_row_order = z +
                                        (y * extent_z) +
                                        (local_index_x *
                                          extent_y * extent_z);
        int local_x                   = local_index_x;
        int local_y                   = y;
        int local_z                   = z;
        auto glob_coords_row =
          pat_blockcyclic_row.global(
            expected_unit_id,
            std::array<index_t, 3> { local_x, local_y, local_z });
        auto glob_coords_col =
          pat_blockcyclic_col.global(
            expected_unit_id,
            std::array<index_t, 3> { local_x, local_y, local_z });
        // Row major:
        EXPECT_EQ(
          expected_unit_id,
          pat_blockcyclic_row.unit_at(std::array<index_t, 3> { x,y,z }));
        EXPECT_TRUE(
          pat_blockcyclic_row.is_local(
            expected_index_row_order,
            unit_id));
        EXPECT_EQ(
          expected_offset_row_order,
          pat_blockcyclic_row.at(std::array<index_t, 3> { x,y,z }));
        EXPECT_EQ(
          (std::array<index_t, 3> { x, y, z }),
          glob_coords_row);
        // Col major:
        EXPECT_EQ(
          expected_unit_id,
          pat_blockcyclic_col.unit_at(std::array<index_t, 3> { x,y,z }));
        EXPECT_TRUE(
          pat_blockcyclic_col.is_local(
            expected_index_col_order,
            unit_id));
        EXPECT_EQ(
          expected_offset_col_order,
          pat_blockcyclic_col.at(std::array<index_t, 3> { x,y,z }));
        EXPECT_EQ(
          (std::array<index_t, 3> { x, y, z }),
          glob_coords_col);
      }
    }
  }
}

TEST_F(BlockPatternTest, LocalExtents2DimCyclicX)
{
  // Must be run on all units as local extents differ

  // 2-dimensional, cyclic partitioning in first dimension:
  //
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] | ... ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] | ... ]
  // [ team 0[4] | team 1[4] | team 0[5] | team 1[5] | ... ]
  // [ team 0[6] | team 1[6] | team 0[7] | team 1[7] | ... ]
  // [                        ...                          ]
  int team_size      = dash::Team::All().size();
  // Two blocks for every unit, plus one block
  int extent_x       = (2 * team_size) + 1;
  int extent_y       = 41;
  int block_size_x   = 1;
  int block_size_y   = extent_y;
//int overflow_bs_x  = 0;
//int overflow_bs_y  = 0;
  int underfill_bs_x = 0;
  int underfill_bs_y = 0;
//size_t size        = extent_x * extent_y;
  int max_per_unit_x = dash::math::div_ceil(extent_x, team_size);
  int max_per_unit   = max_per_unit_x * block_size_y;
  // Unit 0 should have 1 additional block assigned:
  int local_extent_x = (dash::myid() == 0)
                         ? 3
                         : 2;
  int local_extent_y = extent_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpx: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit_x, max_per_unit);
  dash::Pattern<2, dash::ROW_MAJOR> pat_cyclic_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_cyclic_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  // Row major:
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_cyclic_row.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_cyclic_row.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_cyclic_row.local_size(),
            local_extent_x * local_extent_y);
  // Col major:
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_cyclic_col.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_cyclic_col.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_cyclic_col.local_size(),
            local_extent_x * local_extent_y);
}

TEST_F(BlockPatternTest, LocalExtents2DimBlockcyclicY)
{
  // Must be run on all units as local extents differ

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
  //
  // For units 0..n:
  // - unit n has no additional block
  // - unit n-1 has additional block with 1 extent smaller than
  //   regular block size
  // - unit n-2 has additional block with regular block size
  // - all other units have no additional block
  int team_size      = dash::Team::All().size();
  int extent_x       = 41;
  int block_size_y   = 3;
  // Last block is 1 extent smaller:
  int underfill_bs_x = 0;
  int underfill_bs_y = 1;
//int overflow_bs_x  = 0;
//int overflow_bs_y  = block_size_y - underfill_bs_y;
  // Two blocks for every unit, plus one additional block for
  // half of the units:
  int num_add_blocks = dash::math::div_ceil(team_size, 2);
  int min_blocks_y   = 2 * team_size;
  int num_blocks_y   = min_blocks_y + num_add_blocks;
  int extent_y       = (num_blocks_y * block_size_y) - underfill_bs_y;
  int block_size_x   = extent_x;
  // First half of team should have 1 additional block assigned:
  int local_extent_x = extent_x;
  int local_min_ex_y = (min_blocks_y / team_size) * block_size_y;
  int local_extent_y = local_min_ex_y;
  if (dash::myid() < num_add_blocks) {
    // Unit has additional block:
    local_extent_y += block_size_y;
    if (dash::myid() == num_add_blocks - 1) {
      // Unit has additional underfilled block:
      local_extent_y -= underfill_bs_y;
    }
  }
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, nby: %d, "
              "aby: %d, lex: %d, ley: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      num_blocks_y, num_add_blocks,
      local_extent_x, local_extent_y);
  dash::Pattern<2, dash::ROW_MAJOR> pat_blockcyclic_row(
      dash::SizeSpec<2>(extent_y, extent_x),
      dash::DistributionSpec<2>(
        dash::BLOCKCYCLIC(block_size_y), dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<2>(extent_y, extent_x),
      dash::DistributionSpec<2>(
        dash::BLOCKCYCLIC(block_size_y), dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  // Row major:
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(1), underfill_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(0), underfill_bs_y);
  ASSERT_EQ(pat_blockcyclic_row.local_extent(1), local_extent_x);
  ASSERT_EQ(pat_blockcyclic_row.local_extent(0), local_extent_y);
  EXPECT_EQ(pat_blockcyclic_row.local_size(),
            local_extent_x * local_extent_y);
  // Col major:
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(1), underfill_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(0), underfill_bs_y);
  ASSERT_EQ(pat_blockcyclic_col.local_extent(1), local_extent_x);
  ASSERT_EQ(pat_blockcyclic_col.local_extent(0), local_extent_y);
  EXPECT_EQ(pat_blockcyclic_col.local_size(),
            local_extent_x * local_extent_y);
}

