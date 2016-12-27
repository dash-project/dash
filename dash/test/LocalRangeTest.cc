
#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "LocalRangeTest.h"


TEST_F(LocalRangeTest, ArrayBlockcyclic)
{
  if (dash::myid().id == 0) {
    dart_domain_locality_t * glob_loc_dom;
    dart_domain_team_locality(
      DART_TEAM_ALL, ".", &glob_loc_dom);
    std::cout << (dash::util::LocalityJSONPrinter() << *glob_loc_dom).str()
              << std::endl;
  }
  dash::barrier();

  return;

  const size_t blocksize        = 3;
  const size_t num_blocks_local = 2;
  const size_t num_elem_local   = num_blocks_local * blocksize;
  size_t num_elem_total         = _dash_size * num_elem_local;
  // Identical distribution in all ranges:
  dash::Array<int> array(num_elem_total,
                         dash::BLOCKCYCLIC(blocksize));
  // Should return full local index range from 0 to 6:
  auto l_idx_range_full = dash::local_index_range(
                            array.begin(),
                            array.end());

  ASSERT_EQ_U(l_idx_range_full.begin, 0);
  ASSERT_EQ_U(l_idx_range_full.end, 6);
  // Local index range from second half of global range, so every unit should
  // start its local range from the second block:
  LOG_MESSAGE("array.size: %d", array.size());
  auto l_idx_range_half = dash::local_index_range(
                            array.begin() + (array.size() / 2),
                            array.end());
  LOG_MESSAGE("Local index range: lbegin:%d lend:%d",
              l_idx_range_half.begin,
              l_idx_range_half.end);
  ASSERT_EQ_U(3, l_idx_range_half.begin);
  ASSERT_EQ_U(6, l_idx_range_half.end);
}

TEST_F(LocalRangeTest, ArrayBlockedWithOffset)
{
  if (_dash_size < 2) {
    return;
  }

  const size_t block_size      = 20;
  const size_t num_elems_total = _dash_size * block_size;
  // Start at global index 5:
  const size_t offset          = 5;
  // Followed by 2.5 blocks:
  const size_t num_elems       = (block_size * 2) + (block_size / 2);

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  LOG_MESSAGE("global index range: begin:%ld end:%ld",
              offset, offset + num_elems);
  auto l_idx_range = dash::local_index_range(
                       array.begin() + offset,
                       array.begin() + offset + num_elems);
  LOG_MESSAGE("local index range: begin:%d - end:%d",
              l_idx_range.begin, l_idx_range.end);
  if (dash::myid() == 0) {
    // Local range of unit 0 should start at offset:
    ASSERT_EQ_U(offset, l_idx_range.begin);
  } else if (dash::myid() == 1) {
    // Local range of unit 1 should span full local range:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(block_size, l_idx_range.end);
  } else if (dash::myid() == 2) {
    // Local range of unit 3 should span 5 units (offset) plus half a block:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(offset + (block_size / 2), l_idx_range.end);
  } else {
    // All other units should have an empty local range:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(0, l_idx_range.end);
  }
}

TEST_F(LocalRangeTest, View2DimRange)
{
  const size_t block_size_x  = 3;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t num_blocks_x        = _dash_size * 2;
  size_t num_blocks_y        = _dash_size * 2;
  size_t num_blocks_total    = num_blocks_x * num_blocks_y;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / _dash_size;
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;

  LOG_MESSAGE("nunits:%ld "
              "elem_total:%ld elem_per_unit:%ld blocks_per_unit:%ld",
              _dash_size,
              num_elem_total, num_elem_per_unit, num_blocks_per_unit);

  typedef int                                            element_t;
  typedef dash::TilePattern<2>                           pattern_t;
  typedef typename pattern_t::index_type                 index_t;
  typedef dash::Matrix<element_t, 2, index_t, pattern_t> matrix_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_x,
      extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y))
  );

  matrix_t matrix(pattern);

  int lb = 0;
  for (int b = 0; b < static_cast<int>(num_blocks_total); ++b) {
    auto g_block        = matrix.block(b);
    auto g_block_first  = g_block.begin();
    auto g_block_view   = g_block_first.viewspec();
    auto g_block_begin  = g_block.begin();
    auto g_block_end    = g_block.end();
    auto g_block_region = g_block_view.region();
    DASH_LOG_DEBUG("LocalRangeTest.View2DimRange", "block", b, "view region:",
                   g_block_region.begin, g_block_region.end);
    LOG_MESSAGE("Checking if block %d is local", b);
    if (g_block_first.is_local()) {
      LOG_MESSAGE("Block %d is local (local block: %d)", b, lb);
      auto block            = matrix.local.block(lb);
      LOG_MESSAGE("Resolving iterator range of block %d (local: %d)", b, lb);
      auto block_begin      = block.begin();
      auto block_end        = block.end();
      auto block_begin_view = block_begin.viewspec();
      auto block_end_view   = block_end.viewspec();
      LOG_MESSAGE("block.begin() pos:%d view: offset:(%d,%d) extent:(%d,%d)",
                  block_begin.pos(),
                  block_begin_view.offset(0), block_begin_view.offset(1),
                  block_begin_view.extent(0), block_begin_view.extent(1));
      LOG_MESSAGE("block.end()   pos:%d view: offset:(%d,%d) extent:(%d,%d)",
                  block_end.pos(),
                  block_end_view.offset(0), block_end_view.offset(1),
                  block_end_view.extent(0), block_end_view.extent(1));
      LOG_MESSAGE("Index range of block: global: (%d..%d] local: (%d..%d]",
                  block_begin.gpos(), block_end.gpos(),
                  block_begin.pos(),  block_end.pos());
      LOG_MESSAGE("Resolving local index range in local block");
      // Local index range of first local block should return local index range
      // of block unchanged:
      auto l_idx_range = dash::local_index_range(
                           block_begin,
                           block_end);
      LOG_MESSAGE("Local index range: (%d..%d]",
                  l_idx_range.begin, l_idx_range.end);
      ASSERT_EQ_U(block_begin.pos(), l_idx_range.begin);
      ASSERT_EQ_U(block_end.pos(),   l_idx_range.end);
      ++lb;
    } else {
      // Local index range of non-local block should return empty index range:
      LOG_MESSAGE("Resolving local index range in remote block");
      auto l_idx_range = dash::local_index_range(
                           g_block_begin,
                           g_block_end);
      LOG_MESSAGE("Local index range: (%d..%d]",
                  l_idx_range.begin, l_idx_range.end);
      ASSERT_EQ_U(l_idx_range.begin, l_idx_range.end);
    }
  }
}

