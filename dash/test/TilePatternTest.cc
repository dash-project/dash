#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TilePatternTest.h"

TEST_F(TilePatternTest, Tile2DimTeam2Dim)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::default_index_t index_t;

  if (dash::size() % 2 != 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for "
                "TilePatternTest.Tile2DimTeam2Dim");
    return;
  }

  // 2-dimensional, blocked partitioning in first dimension:
  //
  // [ team 0[0] | team 0[1] | ... | team 0[8]  | team 0[9]  | ... ]
  // [ team 0[2] | team 0[3] | ... | team 0[10] | team 0[11] | ... ]
  // [ team 0[4] | team 0[5] | ... | team 0[12] | team 0[13] | ... ]
  // [ team 0[6] | team 0[7] | ... | team 0[14] | team 0[15] | ... ]
  size_t team_size    = dash::Team::All().size();
  size_t team_size_x  = std::max<size_t>(
                          1, static_cast<size_t>(std::ceil(sqrt(team_size))));
  size_t team_size_y  = team_size / team_size_x;
  LOG_MESSAGE("team size: %lu x %lu", team_size_x, team_size_y);

  // Choose 'inconvenient' extents:
  size_t block_size_x = 3;
  size_t block_size_y = 2;
  size_t block_size   = block_size_x * block_size_y;
  size_t extent_x     = team_size_x * block_size_x;
  size_t extent_y     = team_size_y * block_size_y;
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
}

