
#include "SeqTilePatternTest.h"

#include <dash/pattern/SeqTilePattern.h>
#include <dash/pattern/MakePattern.h>

#include <dash/util/PatternMetrics.h>

#include <dash/algorithm/SUMMA.h>

#include <dash/Dimensional.h>
#include <dash/TeamSpec.h>


TEST_F(SeqTilePatternTest, Distribute2DimTile)
{
  DASH_TEST_LOCAL_ONLY();

  typedef dash::SeqTilePattern<2, dash::ROW_MAJOR> pattern_t;
  typedef typename pattern_t::index_type             index_t;

  if (dash::size() % 2 != 0) {
    SKIP_TEST_MSG("Team size must be multiple of 2 for "
                  "SeqTilePatternTest.Distribute2DimTile");
  }

  size_t team_size    = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  size_t block_rows   = 3;
  size_t block_cols   = 2;
  size_t block_size   = block_rows * block_cols;
  size_t size_rows    = (team_size+1) * 3 * block_rows;
  size_t size_cols    = (team_size-1) * 2 * block_cols;
  size_t size         = size_rows * size_cols;

  dash::SizeSpec<2> sizespec(size_rows, size_cols);
  auto teamspec = dash::make_team_spec<
                     dash::summa_pattern_partitioning_constraints,
                     dash::summa_pattern_mapping_constraints,
                     dash::summa_pattern_layout_constraints >(
                       sizespec);

  pattern_t pattern(
    sizespec,
    dash::DistributionSpec<2>(
      dash::TILE(block_rows),
      dash::TILE(block_cols)),
    teamspec, dash::Team::All());

  dash::util::PatternMetrics<pattern_t> pm(pattern);
  for (dash::team_unit_t unit{0}; unit < dash::size(); ++unit) {
    auto unit_local_blocks = pattern.local_blockspec(unit).size();
    LOG_MESSAGE("Blocks mapped to unit %d: %lu",
                unit.id, unit_local_blocks);
    EXPECT_EQ_U(pm.unit_local_blocks(unit), unit_local_blocks);
  }

  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "pattern.row.unit_at", pattern, 3,
    [](const decltype(pattern) & _pattern, int _x, int _y) -> int {
      return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.row.at", pattern, 3,
    [](const decltype(pattern) & _pattern, int _x, int _y) -> index_t {
      return _pattern.at(std::array<index_t, 2> {_x, _y});
    });
    dash::test::print_pattern_mapping(
      "pattern.row.local_index", pattern, 3,
    [](const decltype(pattern) & _pattern, int _x, int _y) -> index_t {
      return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
    });
    dash::test::print_pattern_mapping(
      "pattern.row.local_coords", pattern, 5,
    [](const decltype(pattern) & _pat, int _x, int _y) -> std::string {
      auto l_c = _pat.local_coords(std::array<index_t, 2> {_x, _y});
      std::ostringstream ss;
      ss << l_c[0] << "," << l_c[1];
      return ss.str();
    });
  }

  ASSERT_EQ(dash::TeamSpec<2>(dash::Team::All()).size(), team_size);
  ASSERT_EQ(pattern.capacity(), size);
  ASSERT_EQ(pattern.blocksize(0), block_rows);
  ASSERT_EQ(pattern.blocksize(1), block_cols);
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
  for (int x = 0; x < static_cast<int>(size_cols); ++x) {
    for (int y = 0; y < static_cast<int>(size_rows); ++y) {
    }
  }
}

TEST_F(SeqTilePatternTest, SeqTilePattern1DFunctionalCheck)
{
  auto num_units = dash::Team::All().size();

  //series of default test
  typedef dash::SeqTilePattern<1> pattern_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_pattern<pattern_t>(i);

  // series of unsigned COLUMN MAJOR test
  typedef dash::SeqTilePattern<1, dash::COL_MAJOR, unsigned long> upattern_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_pattern<upattern_t>(i);
}

TEST_F(SeqTilePatternTest, SeqTilePatternFunctionalCheck)
{
  auto num_units = dash::Team::All().size();

  //series of default 2D test
  typedef dash::SeqTilePattern<2> pattern_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_pattern<pattern_t>(i, i);

  // series of unsigned COLUMN MAJOR 2D test
  typedef dash::SeqTilePattern<2, dash::COL_MAJOR, unsigned long> upattern_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_pattern<upattern_t>(i, i);
}
