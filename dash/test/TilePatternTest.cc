#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TilePatternTest.h"

TEST_F(TilePatternTest, Distribute1DimTile)
{
  DASH_TEST_LOCAL_ONLY();

  size_t team_size  = dash::Team::All().size();
  size_t block_size = 3;
  size_t extent     = team_size * (block_size * 3) + 1;
  size_t num_blocks = dash::math::div_ceil(extent, block_size);
  size_t local_cap  = block_size *
                        dash::math::div_ceil(num_blocks, team_size);
  dash::TilePattern<1, dash::ROW_MAJOR> pat_tile_row(
      dash::SizeSpec<1>(extent),
      dash::DistributionSpec<1>(dash::TILE(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Check that memory order is irrelevant for 1-dim
  dash::TilePattern<1, dash::COL_MAJOR> pat_tile_col(
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

  std::array<int, 1> expected_coord;
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
      (std::array<int, 1> { x }),
      pat_tile_row.coords_to_global(
        expected_unit_id, (std::array<int, 1> { expected_offset })));
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
      (std::array<int, 1> { x }),
      pat_tile_col.coords_to_global(
        expected_unit_id, (std::array<int, 1> { expected_offset })));
  }
}

TEST_F(TilePatternTest, Distribute2DimTileX)
{
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ team 0[0] | team 0[1] | ... | team 0[8]  | team 0[9]  | ... ]
  // [ team 0[2] | team 0[3] | ... | team 0[10] | team 0[11] | ... ]
  // [ team 0[4] | team 0[5] | ... | team 0[12] | team 0[13] | ... ]
  // [ team 0[6] | team 0[7] | ... | team 0[14] | team 0[15] | ... ]
  size_t team_size      = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t extent_y       = 7;
  size_t block_size_x   = 3;
  size_t extent_x       = team_size * block_size_x * 2;
  size_t size           = extent_x * extent_y;
  size_t max_per_unit_x = dash::math::div_ceil(extent_x, team_size);
  size_t block_size_y   = extent_y;
  size_t max_per_unit   = max_per_unit_x * block_size_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpx: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit_x, max_per_unit);
  dash::TilePattern<2, dash::ROW_MAJOR> pat_tile_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::TILE(block_size_x),
        dash::TILE(block_size_y)),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::TilePattern<2, dash::COL_MAJOR> pat_tile_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::TILE(block_size_x),
        dash::TILE(block_size_y)),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  ASSERT_EQ(pat_tile_row.capacity(), size);
  ASSERT_EQ(pat_tile_row.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_tile_row.blocksize(0), block_size_x);
  ASSERT_EQ(pat_tile_row.blocksize(1), block_size_y);
  ASSERT_EQ(pat_tile_col.capacity(), size);
  ASSERT_EQ(pat_tile_col.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_tile_col.blocksize(0), block_size_x);
  ASSERT_EQ(pat_tile_col.blocksize(1), block_size_y);
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
  int num_overflow_blocks = extent_x % team_size;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int unit_id                   = (x / block_size_x) % team_size;
      int min_blocks_x              = extent_x / team_size;
      int num_blocks_x              = dash::math::div_ceil(
                                        extent_x, block_size_x);
      int num_add_blocks_x          = num_blocks_x % team_size;
      int block_index_x             = x / block_size_x;
      int local_block_index_x       = block_index_x % team_size;
      int local_index_x             = (local_block_index_x *
                                        block_size_x) +
                                      (x % block_size_x);
      int num_add_elements_x        = 0;
      int overflow_block_size_x     = extent_x % block_size_x;
      int block_elem_index_row      = (x % block_size_x) +
                                      (y * block_size_x);
      int block_elem_index_col      = y + 
                                      ((x % block_size_x) *
                                        block_size_y);
      int block_size_x_act          = block_size_x;
      int num_blocks_unit_x         = min_blocks_x;
      if (unit_id < num_add_blocks_x) {
        num_blocks_unit_x++;
      }
      if (block_index_x == num_blocks_x-1) {
        block_size_x_act = overflow_block_size_x;
      }
      int expected_index_row_order  = (block_index_x *
                                        block_size_x_act * 
                                        block_size_y) +
                                      block_elem_index_row;
      int expected_index_col_order  = (block_index_x *
                                        block_size_x_act * 
                                        block_size_y) +
                                      block_elem_index_col;
      int expected_offset_row_order = (local_block_index_x *
                                        block_size_x_act * 
                                        block_size_y) +
                                      block_elem_index_row;
      int expected_offset_col_order = (local_block_index_x *
                                        block_size_x_act * 
                                        block_size_y) +
                                      block_elem_index_col;
      int local_x                   = local_index_x;
      int local_y                   = y;
      // Row major:
      LOG_MESSAGE("R %d,%d, u:%d, bx:%d, bsxa:%d, nbu:%d, lbx:%d, bei:%d",
        x, y,
        unit_id,
        block_index_x,
        block_size_x_act,
        num_blocks_unit_x,
        local_block_index_x,
        block_elem_index_row);
      auto glob_coords_row = 
        pat_tile_row.coords_to_global(
          unit_id,
          std::array<int, 2> { local_x, local_y });
      EXPECT_EQ(
        unit_id,
        pat_tile_row.unit_at(std::array<int, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_tile_row.at(std::array<int, 2> { x, y }));
      EXPECT_EQ(
        (std::array<int, 2> { x, y }),
        glob_coords_row);
      // Col major:
      LOG_MESSAGE("C %d,%d, u:%d, bx:%d, bsxa:%d, nbu:%d, lbx:%d, bei:%d",
        x, y,
        unit_id,
        block_index_x,
        block_size_x_act,
        num_blocks_unit_x,
        local_block_index_x,
        block_elem_index_col);
      auto glob_coords_col =
        pat_tile_col.coords_to_global(
          unit_id,
          std::array<int, 2> { local_x, local_y });
      EXPECT_EQ(
        unit_id,
        pat_tile_col.unit_at(std::array<int, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_tile_col.at(std::array<int, 2> { x, y }));
      EXPECT_EQ(
        (std::array<int, 2> { x, y }),
        glob_coords_col);
    }
  }
}

