#include <libdash.h>
#include <gtest/gtest.h>
#include <iomanip>

#include "TestBase.h"
#include "TilePatternTest.h"

using std::endl;
using std::setw;

TEST_F(TilePatternTest, Tile2DimTeam2Dim)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::default_index_t  index_t;
  typedef std::array<index_t, 2> coords_t;

  if (dash::size() % 2 != 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for "
                "TilePatternTest.Tile2DimTeam2Dim");
    return;
  }

  size_t team_size    = dash::Team::All().size();
  size_t team_size_x  = std::max<size_t>(
                          1, static_cast<size_t>(std::ceil(
                               sqrt(team_size))));
  size_t team_size_y  = team_size / team_size_x;
  LOG_MESSAGE("team size: %lu x %lu", team_size_x, team_size_y);

  // Choose 'inconvenient' extents:
  int    block_size_x = 2;
  int    block_size_y = 2;
  int    odd_blocks_x = 1;
  int    odd_blocks_y = 2;
  size_t block_size   = block_size_x * block_size_y;
  size_t extent_x     = (team_size_x + odd_blocks_x) * block_size_x;
  size_t extent_y     = (team_size_y + odd_blocks_y) * block_size_y;
  size_t size         = extent_x * extent_y;
  size_t max_per_unit = size / team_size;
  LOG_MESSAGE("e:%d,%d, bs:%d,%d, nu:%d, mpu:%d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      team_size,
      max_per_unit);
  dash__unused(max_per_unit);
  dash__unused(block_size);

  ASSERT_EQ(dash::TeamSpec<2>(dash::Team::All()).size(), team_size);

  dash::TeamSpec<2> teamspec_2d(team_size_x, team_size_y);
  ASSERT_EQ(2,            teamspec_2d.rank());
  ASSERT_EQ(team_size_x,  teamspec_2d.num_units(0));
  ASSERT_EQ(team_size_y,  teamspec_2d.num_units(1));
  ASSERT_EQ(dash::size(), teamspec_2d.size());

  dash::TilePattern<2, dash::ROW_MAJOR> pattern(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::TILE(block_size_x),
        dash::TILE(block_size_y)),
      teamspec_2d,
      dash::Team::All());

  // Test .unit_at:

  std::vector< std::vector<dart_unit_t> > pattern_units;
  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    std::vector<dart_unit_t> row_units;
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      row_units.push_back(pattern.unit_at(std::array<index_t, 2> { x, y }));
    }
    pattern_units.push_back(row_units);
  }
  for (auto row_units : pattern_units) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_units);
  }

  // Test .global:

  std::vector< std::vector<std::string> > pattern_g_coords;
  std::vector< std::vector<index_t> >     pattern_g_indices;
  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    std::vector<std::string> row_g_coords;
    std::vector<index_t>     row_g_indices;
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      auto     unit_id  = pattern.unit_at(std::array<index_t, 2> { x, y });
      auto     l_pos    = pattern.local(coords_t { x, y });
      coords_t l_coords = l_pos.coords;
      coords_t g_coords = pattern.global(unit_id, l_coords);
      ASSERT_EQ_U(unit_id, l_pos.unit);
      ASSERT_EQ_U((coords_t { x, y }), g_coords);

      std::ostringstream ss;
      ss << "(" << setw(2) << g_coords[0]
         << "," << setw(2) << g_coords[1] << ")";
      row_g_coords.push_back(ss.str());
      row_g_indices.push_back(pattern.global_index(unit_id, l_coords));
    }
    pattern_g_coords.push_back(row_g_coords);
    pattern_g_indices.push_back(row_g_indices);
  }
  for (auto row_g_indices : pattern_g_indices) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_g_indices);
  }
  for (auto row_g_coords : pattern_g_coords) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_g_coords);
  }

  // Test .local:

  std::vector< std::vector<std::string> > pattern_l_coords;
  std::vector< std::vector<std::string> > pattern_l_indices;
  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    std::vector<std::string> row_l_coords;
    std::vector<std::string> row_l_indices;
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      coords_t g_coords = { x, y };

      auto l_pos_coords = pattern.local(g_coords);
      auto unit_id_c    = l_pos_coords.unit;
      auto l_coords     = l_pos_coords.coords;
      std::ostringstream sc;
      sc << "(" << setw(2) << l_coords[0]
         << "," << setw(2) << l_coords[1] << ")";
      row_l_coords.push_back(sc.str());

      auto l_pos_index  = pattern.local_index(g_coords);
      auto unit_id_i    = l_pos_index.unit;
      auto l_index      = l_pos_index.index;
      auto l_coords_idx = pattern.local_at(l_coords);

      ASSERT_EQ_U(l_index, l_coords_idx);

      std::ostringstream si;
      si << "(" << setw(2) << unit_id_i
         << ":" << setw(2) << l_index   << ")";
      row_l_indices.push_back(si.str());

      ASSERT_EQ_U(unit_id_c, unit_id_i);
    }
    pattern_l_coords.push_back(row_l_coords);
    pattern_l_indices.push_back(row_l_indices);
  }
  for (auto row_l_coords : pattern_l_coords) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_l_coords);
  }
  for (auto row_l_indices : pattern_l_indices) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_l_indices);
  }

  // Test .block:

  std::vector< std::vector<std::string> > pattern_g_blocks;
  std::vector< std::vector<std::string> > pattern_l_blocks;
  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    std::vector<std::string> row_g_blocks;
    std::vector<std::string> row_l_blocks;
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      coords_t g_coords      = { x, y };
//    coords_t l_coords      = { x % block_size_x, y % block_size_y };
      auto     unit_id       = pattern.unit_at(g_coords);
      auto     g_block_index = pattern.block_at(g_coords);
      auto     l_block_index = 0;

      auto g_block_view = pattern.block(g_block_index);
      std::ostringstream sg;
      sg << "("
         << g_block_view.offset(0) << "," << g_block_view.offset(1)
         << ")";
      row_g_blocks.push_back(sg.str());

      auto l_block_view = pattern.local_block(unit_id, l_block_index);
      std::ostringstream sl;
      sl << "("
         << l_block_view.offset(0) << "," << l_block_view.offset(1)
         << ")";
      row_l_blocks.push_back(sl.str());
    }
    pattern_g_blocks.push_back(row_g_blocks);
    pattern_l_blocks.push_back(row_l_blocks);
  }
  for (auto row_g_blocks : pattern_g_blocks) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_g_blocks);
  }
  for (auto row_l_blocks : pattern_l_blocks) {
    DASH_LOG_DEBUG_VAR("TilePatternTest.Tile2DimTeam2Dim", row_l_blocks);
  }

}

