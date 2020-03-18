
#include "TilePatternTest.h"

#include <dash/pattern/TilePattern.h>
#include <dash/TeamSpec.h>

#include <iomanip>
#include <array>


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
  LOG_MESSAGE("e:%zu,%zu, bs:%d,%d, nu:%zu, mpu:%zu",
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


TEST_F(TilePatternTest, Tile4Dim)
{
  using pattern_t = typename dash::TilePattern<4>;
  using IndexType = typename pattern_t::index_type;
  dash::TeamSpec<2> teamspec2d(dash::size(), 1);
  teamspec2d.balance_extents();
  // no distribution in the last two dimensions
  dash::TeamSpec<4> teamspec4d(teamspec2d.extent(0), teamspec2d.extent(1), 1, 1);

  // tile size: each tile has 2x2 elements
  int    tile_size_0 = 2;
  int    tile_size_1 = 2;

  // super-block sizes (tiles of tiles): each super-block has 2x2 local tiles
  int   sblock_size_0 = 2;
  int   sblock_size_1 = 2;
  // N^2 tiles
  int   ntiles_0      = 4*dash::size();
  int   ntiles_1      = 4*dash::size();
  size_t block_size   = sblock_size_1 * sblock_size_0;

  /*
   * Build a pattern that looks like this (00 is tile 0 on unit 0,
   * 13 is tile 3 on unit 1, etc):
    +----------------------------------------+--+--+--+--+
    |00|00 | 01|01 | 10|10 | 11|11 | 04|04|  |  |  |  |  |
    +----------------------------------------------------+
    |00|00 | 01|01 | 10|10 | 11|11 | 04|04|  |  |  |  |  |
    +----------------------------------------------------+
    |02|02 | 03|03 | 12|12 | 13|13 |   |  |  |  |  |  |  |
    +----------------------------------------------------+
    |02|02 | 03|03 | 12|12 | 13|13 |   |  |  |  |  |  |  |
    +----------------------------------------------------+
    |  |   |   |   |   |   |   |   |   |  |  |  |  |  |  |
    +----------------------------------------------------+
    [...]
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   *
   */
  pattern_t pattern(
      ntiles_0, ntiles_1,
      dash::TILE(sblock_size_0),
      dash::TILE(sblock_size_1),
      tile_size_0, tile_size_1,
      dash::TILE(tile_size_0),
      dash::TILE(tile_size_1),
      teamspec4d,
      dash::Team::All());

  for (int d = 0; d < 4; ++d) {
    std::cout << "pattern.extents(" << d << "): " << pattern.extent(d) << std::endl;
  }

  ASSERT_EQ_U(pattern.extent(0)*pattern.extent(2), ntiles_0 * tile_size_0);
  ASSERT_EQ_U(pattern.extent(1)*pattern.extent(3), ntiles_1 * tile_size_1);

  /* check conversion between coords and global index */
  for (IndexType i = 0; i < pattern.extent(0); ++i) {
    for (IndexType j = 0; j < pattern.extent(1); ++j) {
      for (IndexType k = 0; k < pattern.extent(2); ++k) {
        for (IndexType l = 0; l < pattern.extent(3); ++l) {
          std::array<IndexType, 4> global_coords{i, j, k, l};
          auto gidx = pattern.global_at(global_coords);
          auto pattern_coords = pattern.coords(gidx);
          for (int d = 0; d < pattern_coords.size(); ++d) {
            ASSERT_EQ_U(pattern_coords[d], global_coords[d]);
          }
        }
      }
    }
  }

  /* check a few distinct coordinates with known indeces */

  // second element in the third block on the first row
  ASSERT_EQ_U(pattern.global_at({0, 2, 0, 1}), 17);
  // first element in the third row of tiles
  ASSERT_EQ_U(pattern.global_at({2, 0, 0, 0}),
              pattern.extent(1)*pattern.extent(2)*pattern.extent(3)*2);
  // last element in the first super-block
  ASSERT_EQ_U(pattern.global_at({1, 1, 1, 1}),
              (tile_size_0*tile_size_1*sblock_size_0*sblock_size_1)-1);
  // last element in the last super-block
  ASSERT_EQ_U(pattern.global_at({ntiles_0-1, ntiles_1-1,
                                  tile_size_0-1, tile_size_1-1}),
              pattern.size()-1);

  /* check conversion from global to local coordinates and back */
  for (IndexType i = 0; i < pattern.extent(0); ++i) {
    for (IndexType j = 0; j < pattern.extent(1); ++j) {
      for (IndexType k = 0; k < pattern.extent(2); ++k) {
        for (IndexType l = 0; l < pattern.extent(3); ++l) {
          std::array<IndexType, 4> global_coords{i, j, k, l};
          typename pattern_t::local_coords_t l_coords = pattern.local(global_coords);
          auto pattern_coords = pattern.global(l_coords.unit, l_coords.coords);
          for (int d = 0; d < pattern_coords.size(); ++d) {
            ASSERT_EQ_U(pattern_coords[d], global_coords[d]);
          }
        }
      }
    }
  }
}

template<dash::dim_t NumDimensions = 1, dash::MemArrange Arangement = dash::ROW_MAJOR, typename IndexType = dash::default_index_t, typename ... Args>
void test_tile_pattern(typename std::make_unsigned<IndexType>::type array_size, Args &&... args) // array_size is first dim size 
{
  using namespace std;
  typedef typename std::make_unsigned<IndexType>::type                     SizeType;
  typedef typename dash::TilePattern<NumDimensions, Arangement, IndexType> pattern_t;

  typedef typename dash::SizeSpec<NumDimensions, SizeType>                 SizeSpec_t;
  typedef typename dash::DistributionSpec<NumDimensions>                   DistributionSpec_t;
  typedef typename dash::TeamSpec<NumDimensions, IndexType>                TeamSpec_t;

  typedef typename std::array<IndexType, NumDimensions>                    coords_t;

  auto num_units = dash::Team::All().size();
  auto unit_id   = dash::myid();

  coords_t coord = {0, static_cast<IndexType>(!(bool) args)...};

  // test constructors
  pattern_t pattern1(array_size, args...); // check compiling, not check equality with other constructor due to the difference in DistributionSpec

  pattern_t pattern2(SizeSpec_t(array_size, args...),
                     DistributionSpec_t(dash::TILE(array_size / num_units), dash::TILE(args / num_units)...));
  
  pattern_t pattern3(SizeSpec_t(array_size, args...),
                     DistributionSpec_t(dash::TILE(array_size / num_units), dash::TILE(args / num_units)...),
                     TeamSpec_t(num_units, static_cast<int>((bool) args)...)); // a hack to input default 1 for higher dims depended on number of args

  pattern_t pattern4 = pattern3;

  ASSERT_EQ_U(pattern2 == pattern3, true);
  ASSERT_EQ_U(pattern3 == pattern4, true);

  //test .extents
  ASSERT_EQ_U(pattern2.extent(0), array_size);

  auto l_extents = pattern2.local_extents();
  ASSERT_EQ_U(accumulate(begin(l_extents), end(l_extents), 1, multiplies<SizeType>()), pattern2.local_size());

  auto g_extents = pattern2.extents();
  ASSERT_EQ_U(accumulate(begin(g_extents), end(g_extents), 1, multiplies<SizeType>()), pattern2.size());

  ASSERT_EQ_U(pattern2.local_size(), pattern2.local_capacity()); //assumed balanced extents
  ASSERT_EQ_U(pattern2.size(), pattern2.capacity());

  //test .at
  ASSERT_EQ_U(pattern2.at(coord), 0);

  //test .is_local
  auto l_pos = pattern2.local(coord);
  ASSERT_EQ_U(pattern2.is_local(pattern2.global_index(pattern2.unit_at(coord), l_pos.coords)), unit_id == 0);

  //test BlockSpec, Local BlockSpec, .blocksize, .max_blocksize
  auto block_spec = pattern2.blockspec();
  auto l_block_spec = pattern2.local_blockspec();

  int nblocks =  static_cast<int>(pow(num_units, NumDimensions));

  ASSERT_EQ_U(block_spec.size(), nblocks);
  ASSERT_EQ_U(l_block_spec.size(), nblocks / num_units); //assumed balanced extents

  int blocksize = 1;
  for (int dim = 0; dim < NumDimensions; dim++) 
    blocksize *= pattern2.blocksize(dim);
  ASSERT_EQ_U(blocksize, pattern2.max_blocksize());

  //test .coords
  IndexType idx = 0;
  ASSERT_EQ_U(pattern2.coords(idx), coord);


  //test .sizespec, .teamspec
  auto size_spec = pattern2.sizespec();
  auto team_spec = pattern2.teamspec();

  ASSERT_EQ_U(size_spec.extent(0), array_size);
  ASSERT_EQ_U(team_spec.size(), num_units);
}

TEST_F(TilePatternTest, TilePattern1DFunctionalCheck)
{
  auto num_units = dash::Team::All().size();

  //series of default test
  typedef std::make_unsigned<dash::default_index_t>::type size_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_tile_pattern(i);

  // series of unsigned COLUMN MAJOR test
  typedef unsigned long                                   index_t;
  typedef std::make_unsigned<index_t>::type               usize_t;
  for (usize_t i = num_units; i <= 1000 * num_units; i *= 10) test_tile_pattern<1, dash::COL_MAJOR, index_t>(i);
}

TEST_F(TilePatternTest, TilePatternFunctionalCheck)
{
  auto num_units = dash::Team::All().size();

  //series of default 2D test
  typedef std::make_unsigned<dash::default_index_t>::type size_t;
  for (size_t i = num_units; i <= 1000 * num_units; i *= 10) test_tile_pattern<2>(i, i);

  // series of unsigned COLUMN MAJOR 2D test
  typedef unsigned long                                   index_t;
  typedef std::make_unsigned<index_t>::type               usize_t;
  for (usize_t i = num_units; i <= 1000 * num_units; i *= 10) test_tile_pattern<2, dash::COL_MAJOR, index_t>(i, i);
}