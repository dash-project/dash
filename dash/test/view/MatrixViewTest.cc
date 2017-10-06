
#include "MatrixViewTest.h"
#include "ViewTestBase.h"

#include <gtest/gtest.h>

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/Meta.h>
#include <dash/Pattern.h>

#include <array>
#include <string>


using dash::test::range_str;
using dash::test::print_nview;
using dash::test::region_values;

TEST_F(MatrixViewTest, GlobalSubLocalBlocks)
{
  using namespace dash;

  typedef dash::SeqTilePattern<2>          pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  auto myid   = dash::Team::All().myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * (nunits + 1);
  size_t extent_y            = block_size_y * (nunits + 1);

  dash::TeamSpec<2> teamspec(dash::Team::All());
  teamspec.balance_extents();

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_y,
      extent_x),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_y),
      dash::TILE(block_size_x)),
    teamspec
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix(pattern);

  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });

  dash::barrier();

  // --------------------------------------------------------------------
  // matrix | sub
  //
  auto matrix_sub = matrix | sub<0>(3, extent_y-1)
                           | sub<1>(1, extent_x-1);

  EXPECT_EQ_U(matrix.extent(0) - 4, matrix_sub.extent(0));
  EXPECT_EQ_U(matrix.extent(1) - 2, matrix_sub.extent(1));

  // --------------------------------------------------------------------
  // matrix | sub | blocks
  //
  {
    auto m_s_blocks     = matrix_sub | blocks();
    auto m_s_blocks_idx = m_s_blocks | index();
    int b_idx = 0;
    for (const auto & blk : m_s_blocks) {
      auto blk_gidx              = m_s_blocks_idx[b_idx];
      auto blk_glob_viewspec     = matrix.pattern().block(blk_gidx);
      bool blk_is_local_expected = matrix.pattern().unit_at(
                                     blk_glob_viewspec.offsets()
                                   ) == myid;
      bool blk_is_strid_expected = !test::is_contiguous_ix(blk | index());

      EXPECT_EQ_U(blk_is_local_expected, blk.is_local_at(myid));
      EXPECT_EQ_U(blk_is_strid_expected, blk.is_strided());
      ++b_idx;
    }
  }

  // --------------------------------------------------------------------
  // matrix | sub | local | blocks
  //
  return;
  {
    auto m_s_l_blocks     = matrix_sub   | local() | blocks();
    auto m_s_l_blocks_idx = m_s_l_blocks | index();
    int b_idx = 0;
    for (const auto & blk : m_s_l_blocks) {
      auto blk_gidx              = m_s_l_blocks_idx[b_idx];
      auto blk_glob_viewspec     = matrix.pattern().block(blk_gidx);
      bool blk_is_local_expected = true;
      bool blk_is_strid_expected = !test::is_contiguous_ix(blk | index());

      ++b_idx;
    }
  }
}

