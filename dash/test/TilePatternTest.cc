#include <libdash.h>
#include <gtest/gtest.h>
#include <iomanip>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "TilePatternTest.h"

using std::endl;
using std::setw;

TEST_F(TilePatternTest, Tile2DimTeam2Dim)
{
  typedef dash::default_index_t                 index_t;
  typedef std::array<index_t, 2>                coords_t;
  typedef dash::TilePattern<2, dash::ROW_MAJOR> pattern_t;

  if (dash::size() % 2 != 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for "
                "TilePatternTest.Tile2DimTeam2Dim");
    return;
  }

  size_t team_size    = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  size_t team_size_x  = teamspec_2d.num_units(0);
  size_t team_size_y  = teamspec_2d.num_units(1);
  int    team_rank    = (team_size_x > 1 && team_size_y > 1) ? 2 : 1;

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

  ASSERT_EQ_U(dash::TeamSpec<2>(dash::Team::All()).size(), team_size);

  ASSERT_EQ(team_rank,    teamspec_2d.rank());
  ASSERT_EQ(dash::size(), teamspec_2d.size());

  pattern_t pattern(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(
        dash::TILE(block_size_x),
        dash::TILE(block_size_y)),
      teamspec_2d,
      dash::Team::All());

  // Test .unit_at:

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.unit_at", pattern, 2,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _pattern.unit_at(coords_t {_x, _y});
      });
  }

  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      auto     unit_id  = pattern.unit_at(std::array<index_t, 2> { x, y });
      auto     l_pos    = pattern.local(coords_t { x, y });
      coords_t l_coords = l_pos.coords;
      coords_t g_coords = pattern.global(unit_id, l_coords);
      ASSERT_EQ_U(unit_id, l_pos.unit);
      ASSERT_EQ_U((coords_t { x, y }), g_coords);
    }
  }

  // Test .local:

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.local", pattern, 7,
      [](const pattern_t & _pattern, int _x, int _y) -> std::string {
          auto lpos    = _pattern.local(coords_t {_x, _y});
          auto unit    = lpos.unit;
          auto lcoords = lpos.coords;
          std::ostringstream ss;
          ss << "u" << unit << "("
             << lcoords[0] << ","
             << lcoords[1]
             << ")";
          return ss.str();
      });
    dash::test::print_pattern_mapping(
      "pattern.local_index", pattern, 6,
      [](const pattern_t & _pattern, int _x, int _y) -> std::string {
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
  dash::test::print_pattern_mapping(
    "pattern.local_at", pattern, 6,
    [](const pattern_t & _pattern, int _x, int _y) -> std::string {
        auto lpos    = _pattern.local(coords_t {_x, _y});
        auto unit    = lpos.unit;
        auto lcoords = lpos.coords;
        std::ostringstream ss;
        ss << "u" << unit << "(";
        if (unit == _pattern.team().myid()) {
          auto lindex = _pattern.local_at(lcoords);
          ss << std::setw(2) << lindex;
        } else {
          ss << std::setw(2) << "--";
        }
        ss << ")";
        return ss.str();
    });

  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
      coords_t g_coords = { x, y };

      auto l_pos_coords = pattern.local(g_coords);
      auto unit_id_c    = l_pos_coords.unit;
      auto l_coords     = l_pos_coords.coords;

      auto l_pos_index  = pattern.local_index(g_coords);
      auto unit_id_i    = l_pos_index.unit;
      auto l_index      = l_pos_index.index;

      ASSERT_EQ_U(unit_id_c, unit_id_i);

      if (pattern.team().myid() == unit_id_i) {
        auto l_coords_idx = pattern.local_at(l_coords);
        ASSERT_EQ_U(l_index, l_coords_idx);
      }
    }
  }

  // Test .global:

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.global", pattern, 7,
      [](const pattern_t & _pattern, int _x, int _y) -> std::string {
          auto unit    = _pattern.unit_at(coords_t { _x, _y });
          auto gcoords = _pattern.global(unit, coords_t {_x, _y});
          std::ostringstream ss;
          ss << "(" << gcoords[0] << "," << gcoords[1] << ")";
          return ss.str();
      });
  }

  // Test .block:

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.block_at.offset", pattern, 7,
      [](const pattern_t & _pattern, int _x, int _y) -> std::string {
          auto g_block_index = _pattern.block_at(coords_t {_x, _y});
          auto block_v       = _pattern.block(g_block_index);
          std::ostringstream ss;
          ss << "(" << block_v.offset(0) << "," << block_v.offset(1) << ")";
          return ss.str();
      });
  }

  for (int y = 0; y < static_cast<int>(extent_y); ++y) {
    for (int x = 0; x < static_cast<int>(extent_x); ++x) {
/*
      coords_t g_coords      = { x, y };
      auto     unit_id       = pattern.unit_at(g_coords);
      auto     g_block_index = pattern.block_at(g_coords);
      auto     l_block_index = 0;

      auto g_block_view = pattern.block(g_block_index);
      auto l_block_view = pattern.local_block(unit_id, l_block_index);
*/
    }
  }

}

