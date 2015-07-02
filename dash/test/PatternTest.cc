#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "PatternTest.h"

TEST_F(PatternTest, SimpleConstructor)
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

TEST_F(PatternTest, EqualityComparison)
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

TEST_F(PatternTest, CopyConstructorAndAssignment)
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

TEST_F(PatternTest, Distribute1DimBlocked)
{
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional blocked partitioning:
  //
  // [ .. team 0 .. | .. team 1 .. | ... | team n-1 ]
  size_t team_size  = dash::Team::All().size();
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
  
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int expected_unit_id = x / block_size;
    int expected_offset  = x % block_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    auto glob_coords_row = 
      pat_blocked_row.coords_to_global(
        expected_unit_id, std::array<long long, 1> { expected_offset });
    auto glob_coords_col = 
      pat_blocked_col.coords_to_global(
        expected_unit_id, std::array<long long, 1> { expected_offset });
    LOG_MESSAGE("x: %d, eu: %d, eo: %d",
      x, expected_unit_id, expected_offset);
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_row.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_row.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      glob_coords_row);
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_col.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_col.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      glob_coords_col);
  }
}

TEST_F(PatternTest, Distribute1DimCyclic)
{
  DASH_TEST_LOCAL_ONLY();
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
  size_t expected_unit_id;
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int expected_unit_id = x % team_size;
    int expected_offset  = x / team_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    LOG_MESSAGE("x: %d, eu: %d, eo: %d",
      x, expected_unit_id, expected_offset);
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_row.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_row.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      pat_cyclic_row.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_col.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_col.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      pat_cyclic_col.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
  }
}

TEST_F(PatternTest, Distribute1DimBlockcyclic)
{
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional blocked partitioning:
  //
  // [ team 0 | team 1 | team 0 | team 1 | ... ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = 23;
  size_t num_blocks = dash::math::div_ceil(_num_elem, block_size);
  size_t local_cap  = dash::math::div_ceil(_num_elem, team_size);
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
  size_t expected_unit_id;
  LOG_MESSAGE("num elem: %d, block size: %d, num blocks: %d",
    _num_elem, block_size, num_blocks);
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int block_index       = x / block_size;
    int block_base_offset = block_size * (block_index / team_size);
    int expected_unit_id  = block_index % team_size;
    int expected_offset   = (x % block_size) + block_base_offset;
    int expected_index    = x;
    expected_coords[0]    = x;
    LOG_MESSAGE("x: %d, eu: %d, eo: %d, bi: %d bbo: %d",
      x, expected_unit_id, expected_offset, block_index, block_base_offset);
    // Row major:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_row.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_row.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      pat_blockcyclic_row.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
    // Column major:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_col.unit_at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_col.at(std::array<long long, 1> { x }));
    EXPECT_EQ(
      (std::array<long long, 1> { expected_index }),
      pat_blockcyclic_col.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
  }
}

TEST_F(PatternTest, Distribute1DimTile)
{
  DASH_TEST_LOCAL_ONLY();

  size_t team_size  = dash::Team::All().size();
  size_t block_size = 3;
  size_t extent     = team_size * (block_size * 3) + 1;
  size_t num_blocks = dash::math::div_ceil(extent, block_size);
  size_t local_cap  = dash::math::div_ceil(extent, team_size);
  dash::Pattern<1, dash::ROW_MAJOR> pat_tile_row(
      dash::SizeSpec<1>(extent),
      dash::DistributionSpec<1>(dash::TILE(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Check that memory order is irrelevant for 1-dim
  dash::Pattern<1, dash::COL_MAJOR> pat_tile_col(
      dash::SizeSpec<1>(extent),
      dash::DistributionSpec<1>(dash::TILE(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_tile_row.capacity(), extent);
  EXPECT_EQ(pat_tile_row.blocksize(0), block_size);
  EXPECT_EQ(pat_tile_row.local_capacity(), local_cap);
  EXPECT_EQ(pat_tile_col.capacity(), extent);
  EXPECT_EQ(pat_tile_col.blocksize(0), block_size);
  EXPECT_EQ(pat_tile_col.local_capacity(), local_cap);

  std::array<long long, 1> expected_coord;
  for (int x = 0; x < extent; ++x) {
    expected_coord[0]     = x;
    int expected_unit_id  = (x / block_size) % team_size;
    int block_index       = x / block_size;
    int block_base_offset = block_size * (block_index / team_size);
    int expected_offset   = (x % block_size) + block_base_offset;
    // Row major:
    EXPECT_EQ(
      expected_coord,
      pat_tile_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_tile_row.unit_at(x));
    EXPECT_EQ(
      expected_offset,
      pat_tile_row.at(x));
    EXPECT_EQ(
      (std::array<long long, 1> { x }),
      pat_tile_row.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
    // Column major:
    EXPECT_EQ(
      expected_coord,
      pat_tile_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_tile_col.unit_at(x));
    EXPECT_EQ(
      expected_offset,
      pat_tile_col.at(x));
    EXPECT_EQ(
      (std::array<long long, 1> { x }),
      pat_tile_col.coords_to_global(
        expected_unit_id, (std::array<long long, 1> { expected_offset })));
  }
}

TEST_F(PatternTest, Distribute2DimBlockedY)
{
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
  int extent_x  = 27;
  int extent_y  = 5;
  size_t size   = extent_x * extent_y;
  // Ceil division
  int block_size_x   = extent_x;
  int block_size_y   = dash::math::div_ceil(extent_y, team_size);
  int max_per_unit   = block_size_x * block_size_y;
  int overflow_bs_x  = extent_x % block_size_x;
  int overflow_bs_y  = extent_y % block_size_y;
  int underfill_bs_x = (overflow_bs_x == 0)
                       ? 0
                       : block_size_x - overflow_bs_x;
  int underfill_bs_y = (overflow_bs_y == 0)
                       ? 0
                       : block_size_y - overflow_bs_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit);
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
  EXPECT_EQ(pat_blocked_row.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_row.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_blocked_row.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_blocked_row.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_blocked_row.underfilled_blocksize(1), underfill_bs_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.local_capacity(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_blocked_col.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_blocked_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_blocked_col.underfilled_blocksize(1), underfill_bs_y);
  LOG_MESSAGE("block size: x: %d, y: %d",
    block_size_x, block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      // Units might have empty local range, e.g. when distributing 41 elements
      // to 8 units.
      int num_blocks_y              = dash::math::div_ceil(extent_y, block_size_y);
      // Subtract missing elements in last block if any:
      int underfill_y               = (y >= (num_blocks_y-1) * block_size_y)
                                      ? (block_size_y * num_blocks_y) - extent_y
                                      : 0;
      // Actual extent of block, adjusted for underfilled extent:
      int block_size_y_adj          = block_size_y - underfill_y;
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order =
        expected_index_row_order % max_per_unit;
      int expected_offset_col_order = (y % block_size_y) + (x * block_size_y_adj);
      int expected_unit_id          = y / block_size_y;
      int local_x                   = x;
      int local_y                   = y % block_size_y;
      // Row major:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d, bya: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.at(std::array<long long, 2> { x, y }),
        expected_index_row_order,
        block_size_x, block_size_y, block_size_y_adj);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        pat_blocked_row.coords_to_global(
          expected_unit_id, 
          (std::array<long long, 2> { local_x, local_y  })));
      // Col major:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.at(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        pat_blocked_col.coords_to_global(
          expected_unit_id,
          (std::array<long long, 2> { local_x, local_y })));
    }
  }
}

TEST_F(PatternTest, Distribute2DimBlockedX)
{
  DASH_TEST_LOCAL_ONLY();
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
      // Units might have empty local range, e.g. when distributing 41 elements
      // to 8 units.
      int num_blocks_x              = dash::math::div_ceil(extent_x, block_size_x);
      // Subtract missing elements in last block if any:
      int underfill_x               = (x >= (num_blocks_x-1) * block_size_x)
                                      ? (block_size_x * num_blocks_x) - extent_x
                                      : 0;
      // Actual extent of block, adjusted for underfilled extent:
      int block_size_x_adj          = block_size_x - underfill_x;
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order = (x % block_size_x) + (y * block_size_x_adj);
      int expected_offset_col_order = expected_index_col_order % 
                                        max_per_unit;
      int expected_unit_id          = x / block_size_x;
      int local_x                   = x % block_size_x;
      int local_y                   = y;
      // Row major:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, nbx: %d, bx: %d, by: %d, bxa: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.at(std::array<long long, 2> { x, y }),
        num_blocks_x,
        block_size_x, block_size_y, block_size_x_adj);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        pat_blocked_row.coords_to_global(
          expected_unit_id,
          (std::array<long long, 2> { local_x, local_y })));
      // Col major:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.at(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        pat_blocked_col.coords_to_global(
          expected_unit_id,
          (std::array<long long, 2> { local_x, local_y })));
    }
  }
}

TEST_F(PatternTest, Distribute2DimCyclicX)
{
  DASH_TEST_LOCAL_ONLY();
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
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_unit_id = x % team_size;
      int expected_offset_row_order = (y * num_blocks_unit_x) + x / team_size;
      int expected_offset_col_order = ((x / team_size) * extent_y) + y;
      int local_x                   = x / team_size;
      int local_y                   = y;
      auto glob_coords_row = 
        pat_cyclic_row.coords_to_global(
          expected_unit_id,
          std::array<long long, 2> { local_x, local_y });
      auto glob_coords_col =
        pat_cyclic_col.coords_to_global(
          expected_unit_id,
          std::array<long long, 2> { local_x, local_y });
      // Row major:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, of: %d, nbu: %d",
        x, y,
        expected_offset_row_order,
        pat_cyclic_row.at(std::array<long long, 2> { x, y }),
        num_overflow_blocks,
        num_blocks_unit_x);
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_row.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_cyclic_row.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        glob_coords_row);
      // Col major:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, of: %d",
        x, y,
        expected_offset_col_order,
        pat_cyclic_col.at(std::array<long long, 2> { x, y }),
        num_overflow_blocks);
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_col.unit_at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_cyclic_col.at(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        (std::array<long long, 2> { x, y }),
        glob_coords_col);
    }
  }
}

TEST_F(PatternTest, Distribute3DimBlockcyclicX)
{
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] | ... ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] | ... ]
  // [ team 0[4] | team 1[4] | team 0[5] | team 1[5] | ... ]
  // [ team 0[6] | team 1[6] | team 0[7] | team 1[7] | ... ]
  // [                        ...                          ]
  size_t team_size    = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t extent_x     = team_size + 7;
  size_t extent_y     = 5;
  size_t extent_z     = 3;
  size_t size         = extent_x * extent_y * extent_z;
  size_t block_size_x = 2;
  int max_per_unit_x  = dash::math::div_ceil(extent_x, team_size);
  int block_size_y    = extent_y;
  int block_size_z    = extent_z;
  int max_per_unit    = max_per_unit_x * block_size_y * block_size_z;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpx: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit_x, max_per_unit);
  dash::Pattern<3, dash::ROW_MAJOR> pat_blockcyclic_row(
      dash::SizeSpec<3>(extent_x, extent_y, extent_z),
      dash::DistributionSpec<3>(dash::BLOCKCYCLIC(block_size_x), dash::NONE, dash::NONE),
      dash::TeamSpec<3>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<3, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<3>(extent_x, extent_y, extent_z),
      dash::DistributionSpec<3>(dash::BLOCKCYCLIC(block_size_x), dash::NONE, dash::NONE),
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
  int num_overflow_blocks = extent_x % team_size;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      for (int z = 0; z < extent_z; ++z) {
        int unit_id                   = (x / block_size_x) % team_size;
        int min_blocks_x              = (extent_x / block_size_x) / team_size;
        int num_blocks_x              = dash::math::div_ceil(extent_x, block_size_x);
        int num_add_blocks_x          = extent_x % team_size;
        int overflow_block_size_x     = extent_x % block_size_x;
        int num_blocks_unit_x         = min_blocks_x;
        int num_add_elem_x            = 0;
        int block_offset_x            = x / block_size_x;
        if (unit_id < num_add_blocks_x) {
          num_blocks_unit_x++;
          num_add_elem_x += block_size_x;
        }
        if (unit_id == (num_blocks_x-1) % team_size) {
          if (overflow_block_size_x > 0) { 
            num_add_elem_x -= block_size_x - overflow_block_size_x;
          }
        }
        int local_extent_x            = (min_blocks_x * block_size_x) + num_add_elem_x;
        int local_block_index_x       = block_offset_x / team_size;
        int expected_index_row_order  = (z * extent_y * extent_x) + (y * extent_x) + x;
        int expected_index_col_order  = (x * extent_y * extent_z) + (y * extent_z) + z;
        int expected_unit_id          = (x / block_size_x) % team_size;
        int local_index_x             = (local_block_index_x * block_size_x) + (x % block_size_x);
        int expected_offset_row_order = local_index_x +
                                        (y * local_extent_x) +
                                        (z * local_extent_x * extent_y);
        int expected_offset_col_order = z +
                                        (y * extent_z) +
                                        (local_index_x * extent_y * extent_z);
        int local_x                   = local_index_x;
        int local_y                   = y;
        int local_z                   = z;
        auto glob_coords_row = 
          pat_blockcyclic_row.coords_to_global(
            expected_unit_id,
            std::array<long long, 3> { local_x, local_y, local_z });
        auto glob_coords_col =
          pat_blockcyclic_col.coords_to_global(
            expected_unit_id,
            std::array<long long, 3> { local_x, local_y, local_z });
        // Row major:
        LOG_MESSAGE("R %d,%d,%d, u:%d, nbu:%d, nbx:%d, box:%d, lbox:%d, na:%d, lex:%d, lx:%d",
          x, y, z,
          unit_id,
          num_blocks_unit_x,
          num_blocks_x,
          block_offset_x,
          local_block_index_x,
          num_add_elem_x,
          local_extent_x,
          local_index_x);
        EXPECT_EQ(
          expected_unit_id,
          pat_blockcyclic_row.unit_at(std::array<long long, 3> { x, y, z }));
        EXPECT_EQ(
          expected_offset_row_order,
          pat_blockcyclic_row.at(std::array<long long, 3> { x, y, z }));
        EXPECT_EQ(
          (std::array<long long, 3> { x, y, z }),
          glob_coords_row);
        // Col major:
        LOG_MESSAGE("C %d,%d,%d, u:%d, nbu:%d, nbx:%d, box:%d, lbox:%d, na:%d, lex:%d, lx:%d",
          x, y, z,
          unit_id,
          num_blocks_unit_x,
          num_blocks_x,
          block_offset_x,
          local_block_index_x,
          num_add_elem_x,
          local_extent_x,
          local_index_x);
        EXPECT_EQ(
          expected_unit_id,
          pat_blockcyclic_col.unit_at(std::array<long long, 3> { x, y, z }));
        EXPECT_EQ(
          expected_offset_col_order,
          pat_blockcyclic_col.at(std::array<long long, 3> { x, y, z }));
        EXPECT_EQ(
          (std::array<long long, 3> { x, y, z }),
          glob_coords_col);
      }
    }
  }
}

TEST_F(PatternTest, LocalExtents2DimCyclicX)
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
  int overflow_bs_x  = 0;
  int overflow_bs_y  = 0;
  int underfill_bs_x = 0;
  int underfill_bs_y = 0;
  size_t size        = extent_x * extent_y;
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
  EXPECT_EQ(pat_cyclic_col.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_cyclic_col.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_cyclic_row.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_cyclic_row.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_cyclic_row.local_size(),
            local_extent_x * local_extent_y);
  // Col major:
  EXPECT_EQ(pat_cyclic_col.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_cyclic_col.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_cyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_cyclic_col.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_cyclic_col.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_cyclic_col.local_size(),
            local_extent_x * local_extent_y);
}

TEST_F(PatternTest, LocalExtents2DimBlockcyclicY)
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
  int underfill_bs_x = 0;
  // Last block is 1 extent smaller:
  int underfill_bs_y = 1;
  int overflow_bs_x  = 0;
  int overflow_bs_y  = block_size_y - underfill_bs_y;
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
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::NONE, dash::BLOCKCYCLIC(block_size_y)),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::NONE, dash::BLOCKCYCLIC(block_size_y)),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  // Row major:
  EXPECT_EQ(pat_blockcyclic_col.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_blockcyclic_row.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_blockcyclic_row.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_blockcyclic_row.local_size(),
            local_extent_x * local_extent_y);
  // Col major:
  EXPECT_EQ(pat_blockcyclic_col.overflow_blocksize(0), overflow_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.overflow_blocksize(1), overflow_bs_y);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(0), underfill_bs_x);
  EXPECT_EQ(pat_blockcyclic_col.underfilled_blocksize(1), underfill_bs_y);
  ASSERT_EQ(pat_blockcyclic_col.local_extent(0), local_extent_x);
  ASSERT_EQ(pat_blockcyclic_col.local_extent(1), local_extent_y);
  EXPECT_EQ(pat_blockcyclic_col.local_size(),
            local_extent_x * local_extent_y);
}

