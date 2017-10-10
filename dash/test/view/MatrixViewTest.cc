
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
    std::vector<double> m_s_b_values;
    std::vector<double> m_s_values;

    int b_idx = 0;
    for (const auto & blk : m_s_blocks) {
      auto blk_gidx              = m_s_blocks_idx[b_idx];
      auto blk_glob_viewspec     = matrix.pattern().block(blk_gidx);
      bool blk_is_local_expected = matrix.pattern().unit_at(
                                     blk_glob_viewspec.offsets()
                                   ) == myid;
      bool blk_is_local_actual   = blk.is_local_at(myid);
      bool blk_is_strid_expected = blk.extent(1) < block_size_x &&
                                   blk.extent(0) > 1;
      bool blk_is_strid_actual   = blk.is_strided();

      DASH_LOG_DEBUG("MatrixViewTest.GlobalSubLocalBlocks",
                     "block view idx:", b_idx, "-> block gidx:", blk_gidx,
                     ":", range_str(blk));

      if (!blk || !(blk | index())) {
        EXPECT_EQ_U(blk.size(), 0);
        EXPECT_EQ_U((blk | index()).size(), 0);
      } else {
        EXPECT_EQ_U(blk_is_local_expected, blk.is_local_at(myid));
        EXPECT_EQ_U(blk_is_strid_expected, blk.is_strided());

        std::copy(blk.begin(), blk.end(),
                  std::back_inserter(m_s_b_values));
      }
      ++b_idx;
    }

    // Block-wise element order differs from element order in sub-matrix
    // which is canonical. Copy, sort and intersect both ranges to test
    // equality:

    std::copy(matrix_sub.begin(), matrix_sub.end(),
              std::back_inserter(m_s_values));

    DASH_LOG_DEBUG("MatrixViewTest.GlobalSubLocalBlocks",
                   "matrix_sub:", (m_s_values));
    DASH_LOG_DEBUG("MatrixViewTest.GlobalSubLocalBlocks",
                   "copied from blocks:", (m_s_b_values));

    std::sort(m_s_values.begin(),   m_s_values.end());
    std::sort(m_s_b_values.begin(), m_s_b_values.end());

    std::vector<double> m_s_isect;
    std::set_difference(m_s_values.begin(),   m_s_values.end(),
                        m_s_b_values.begin(), m_s_b_values.end(),
                        std::back_inserter(m_s_isect));
    DASH_LOG_DEBUG("MatrixViewTest.GlobalSubLocalBlocks",
                   "intersection:", (m_s_isect));
    EXPECT_EQ_U(0, m_s_isect.size());
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

