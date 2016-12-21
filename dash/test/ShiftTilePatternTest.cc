#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "ShiftTilePatternTest.h"


TEST_F(ShiftTilePatternTest, Distribute1DimTile)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::default_index_t index_t;

  size_t team_size  = dash::Team::All().size();
  size_t block_size = 3;
  size_t extent     = team_size * block_size * 2;
  size_t num_blocks = dash::math::div_ceil(extent, block_size);
  size_t local_cap  = block_size *
                      dash::math::div_ceil(num_blocks, team_size);
  dash::ShiftTilePattern<1, dash::ROW_MAJOR> pat_tile_row(
    dash::SizeSpec<1>(extent),
    dash::DistributionSpec<1>(dash::TILE(block_size)),
    dash::TeamSpec<1>(),
    dash::Team::All());
  // Check that memory order is irrelevant for 1-dim
  dash::ShiftTilePattern<1, dash::COL_MAJOR> pat_tile_col(
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

  std::array<index_t, 1> expected_coord;
  for (int x = 0; x < static_cast<int>(extent); ++x) {
    expected_coord[0]         = x;
    dash::team_unit_t expected_unit_id((x / block_size) % team_size);
    index_t block_index       = x / block_size;
    index_t block_base_offset = block_size * (block_index / team_size);
    index_t expected_offset   = (x % block_size) + block_base_offset;
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
      (std::array<index_t, 1> { x }),
      pat_tile_row.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
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
      (std::array<index_t, 1> { x }),
      pat_tile_col.global(
        expected_unit_id, (std::array<index_t, 1> { expected_offset })));
  }
}

TEST_F(ShiftTilePatternTest, Distribute2DimTile)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::default_index_t index_t;

  if (dash::size() % 2 != 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for "
                "ShiftTilePatternTest.Distribute2DimTile");
    return;
  }

  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 0[1] | ... | team 0[8]  | team 0[9]  | ... ]
  // [ team 0[2] | team 0[3] | ... | team 0[10] | team 0[11] | ... ]
  // [ team 0[4] | team 0[5] | ... | team 0[12] | team 0[13] | ... ]
  // [ team 0[6] | team 0[7] | ... | team 0[14] | team 0[15] | ... ]
  size_t team_size      = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t block_size_x   = 3;
  size_t block_size_y   = 2;
  size_t block_size     = block_size_x * block_size_y;
  size_t extent_x       = team_size * 3 * block_size_x;
  size_t extent_y       = team_size * 2 * block_size_y;
  size_t size           = extent_x * extent_y;
  size_t max_per_unit   = size / team_size;
  LOG_MESSAGE("e:%d,%d, bs:%d,%d, nu:%d, mpu:%d",
              extent_x, extent_y,
              block_size_x, block_size_y,
              team_size,
              max_per_unit);
  dash::ShiftTilePattern<2, dash::ROW_MAJOR> pat_tile_row(
    dash::SizeSpec<2>(extent_x, extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y)),
    dash::TeamSpec<2>(dash::size()/2, 2),
    dash::Team::All());
  dash::ShiftTilePattern<2, dash::COL_MAJOR> pat_tile_col(
    dash::SizeSpec<2>(extent_x, extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y)),
    dash::TeamSpec<2>(dash::size()/2, 2),
    dash::Team::All());

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.row.unit_at", pat_tile_row, 3,
    [](const decltype(pat_tile_row) & _pattern, int _x, int _y) -> int {
      return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.row.at", pat_tile_row, 3,
    [](const decltype(pat_tile_row) & _pattern, int _x, int _y) -> index_t {
      return _pattern.at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.row.local_index", pat_tile_row, 3,
    [](const decltype(pat_tile_row) & _pattern, int _x, int _y) -> index_t {
      return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
    });
    dash::test::print_pattern_mapping(
      "pattern.row.local_coords", pat_tile_row, 5,
    [](const decltype(pat_tile_row) & _pat, int _x, int _y) -> std::string {
      auto l_c = _pat.local_coords(std::array<index_t, 2> {_x, _y});
      std::ostringstream ss;
      ss << l_c[0] << "," << l_c[1];
      return ss.str();
    });
    dash::test::print_pattern_mapping(
      "pattern.col.unit_at", pat_tile_col, 3,
    [](const decltype(pat_tile_col) & _pattern, int _x, int _y) -> int {
      return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.col.at", pat_tile_col, 3,
    [](const decltype(pat_tile_col) & _pattern, int _x, int _y) -> index_t {
      return _pattern.at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.col.local_index", pat_tile_col, 3,
    [](const decltype(pat_tile_col) & _pattern, int _x, int _y) -> index_t {
      return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
    });
    dash::test::print_pattern_mapping(
      "pattern.col.local_coords", pat_tile_col, 5,
    [](const decltype(pat_tile_col) & _pat, int _x, int _y) -> std::string {
      auto l_c = _pat.local_coords(std::array<index_t, 2> {_x, _y});
      std::ostringstream ss;
      ss << l_c[0] << "," << l_c[1];
      return ss.str();
    });
  }

  ASSERT_EQ(dash::TeamSpec<2>(dash::Team::All()).size(), team_size);
  ASSERT_EQ(pat_tile_row.capacity(), size);
  ASSERT_EQ(pat_tile_row.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_tile_row.blocksize(0), block_size_x);
  ASSERT_EQ(pat_tile_row.blocksize(1), block_size_y);
  ASSERT_EQ(pat_tile_col.capacity(), size);
  ASSERT_EQ(pat_tile_col.local_capacity(), max_per_unit);
  ASSERT_EQ(pat_tile_col.blocksize(0), block_size_x);
  ASSERT_EQ(pat_tile_col.blocksize(1), block_size_y);
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
  for (int x = 0; x < static_cast<int>(extent_x); ++x) {
    for (int y = 0; y < static_cast<int>(extent_y); ++y) {
      int num_blocks_x        = extent_x / block_size_x;
      int num_blocks_y        = extent_y / block_size_y;
      int num_l_blocks_x      = num_blocks_x / team_size;
      int num_l_blocks_y      = num_blocks_y / team_size;
      int block_index_x       = x / block_size_x;
      int block_index_y       = y / block_size_y;
      dash::team_unit_t unit_id((block_index_x + block_index_y) % team_size);
//    int l_block_index_x     = block_index_x / team_size;
      int l_block_index_y     = block_index_y / team_size;
//    int l_block_index_col   = (block_index_y * num_l_blocks_x) +
//                              l_block_index_x;
      int l_block_index_row   = (block_index_x * num_l_blocks_y) +
                                l_block_index_y;
//    int phase_x             = (x % block_size_x);
      int phase_y             = (y % block_size_y);
//    int phase_col           = (y % block_size_y) * block_size_x +
//                              phase_x;
      int phase_row           = (x % block_size_x) * block_size_y +
                                phase_y;
//    int local_x             = l_block_index_x * block_size_x + phase_x;
//    int local_y             = l_block_index_y * block_size_y + phase_y;
//    int local_index_col     = (l_block_index_col * block_size) +
//                              phase_col;
      int local_index_row     = (l_block_index_row * block_size) +
                                phase_row;

      dash__unused(num_l_blocks_y);

      auto local_coords_row   = pat_tile_row.local_coords(
                                  std::array<index_t, 2> { x, y });
//    auto local_coords_col   = pat_tile_col.local_coords(
//                                std::array<index_t, 2> { x, y });
      LOG_MESSAGE("R %d,%d u:%d b:%d,%d nlb:%d,%d lc: %d,%d lbi:%d p:%d",
                  x, y,
                  unit_id.id,
                  block_index_x,       block_index_y,
                  num_l_blocks_x,      num_l_blocks_y,
                  local_coords_row[0], local_coords_row[1],
                  l_block_index_row,
                  phase_row);
      ASSERT_EQ_U(
        unit_id,
        pat_tile_row.unit_at(std::array<index_t, 2> { x, y }));
      ASSERT_EQ_U(
        local_index_row,
        pat_tile_row.at(std::array<index_t, 2> { x, y }));
      ASSERT_EQ_U(
        local_index_row,
        pat_tile_row.local_at(local_coords_row));
      auto glob_coords_row =
        pat_tile_row.global(
          unit_id,
          std::array<index_t, 2> { local_coords_row[0],
                                   local_coords_row[1] });
      ASSERT_EQ_U(
        (std::array<index_t, 2> { x, y }), glob_coords_row);
    }
  }
}

TEST_F(ShiftTilePatternTest, Tile2DimTeam1Dim)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::default_index_t index_t;

  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 0[1] | ... | team 0[8]  | team 0[9]  | ... ]
  // [ team 0[2] | team 0[3] | ... | team 0[10] | team 0[11] | ... ]
  // [ team 0[4] | team 0[5] | ... | team 0[12] | team 0[13] | ... ]
  // [ team 0[6] | team 0[7] | ... | team 0[14] | team 0[15] | ... ]
  size_t team_size      = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t block_size_x   = 3;
  size_t block_size_y   = 2;
  size_t block_size     = block_size_x * block_size_y;
  size_t extent_x       = team_size * 2 * block_size_x;
  size_t extent_y       = team_size * 2 * block_size_y;
  size_t size           = extent_x * extent_y;
  size_t max_per_unit   = size / team_size;
  LOG_MESSAGE("e:%d,%d, bs:%d,%d, nu:%d, mpu:%d",
              extent_x, extent_y,
              block_size_x, block_size_y,
              team_size,
              max_per_unit);
  dash__unused(max_per_unit);
  dash__unused(block_size);

  ASSERT_EQ(dash::TeamSpec<2>(dash::Team::All()).size(), team_size);

  dash::TeamSpec<2> teamspec_1d(dash::Team::All());
  ASSERT_EQ(1,            teamspec_1d.rank());
  ASSERT_EQ(dash::size(), teamspec_1d.num_units(dash::team_unit_t{0}));
  ASSERT_EQ(1,            teamspec_1d.num_units(dash::team_unit_t{1}));
  ASSERT_EQ(dash::size(), teamspec_1d.size());

  dash::ShiftTilePattern<2, dash::ROW_MAJOR> pattern(
    dash::SizeSpec<2>(extent_x, extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y)),
    teamspec_1d,
    dash::Team::All());

  std::vector< std::vector<dart_unit_t> > pattern_units;
  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    std::vector<dart_unit_t> row_units;
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      row_units.push_back(pattern.unit_at(std::array<index_t, 2> { x, y }));
    }
    pattern_units.push_back(row_units);
  }
  for (auto row_units : pattern_units) {
    DASH_LOG_DEBUG_VAR("ShiftTilePatternTest.Tile2DimTeam1Dim", row_units);
  }
}
