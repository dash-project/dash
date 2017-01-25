
#include "NViewTest.h"

#include <gtest/gtest.h>

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <array>


TEST_F(NViewTest, MatrixBlocked1DimLocalView)
{
  auto nunits = dash::size();

  int block_rows = 5;
  int block_cols = 3;

  int nrows = nunits * block_rows;
  int ncols = nunits * block_cols;

  // columns distributed in blocks of same size:
  //
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //
  dash::Matrix<int, 2> mat(
      nrows,      ncols,
      dash::NONE, dash::BLOCKED);

  mat.barrier();

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "Matrix initialized");

  // select first 2 matrix rows:
  auto nview_rows_g = dash::sub<0>(1, 3, mat);
  auto nview_cols_g = dash::sub<1>(2, 7, mat);
  auto nview_cr_s_g = dash::sub<1>(2, 7, nview_rows_g);
  auto nview_rc_s_g = dash::sub<0>(1, 3, nview_cols_g);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "mat ->",
                 "offsets:", mat.offsets(),
                 "extents:", mat.extents(),
                 "size:",    mat.size());

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "sub<0>(1,3, mat) ->",
                 "offsets:", nview_rows_g.offsets(),
                 "extents:", nview_rows_g.extents(),
                 "size:",    nview_rows_g.size());

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "sub<1>(2,7, mat) ->",
                 "offsets:", nview_cols_g.offsets(),
                 "extents:", nview_cols_g.extents(),
                 "size:",    nview_cols_g.size());

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "sub<1>(2,7, sub<0>(1,3, mat) ->",
                 "offsets:", nview_cr_s_g.offsets(),
                 "extents:", nview_cr_s_g.extents(),
                 "size:",    nview_cr_s_g.size());

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimLocalView",
                 "sub<0>(1,3, sub<0>(2,7, mat) ->",
                 "offsets:", nview_rc_s_g.offsets(),
                 "extents:", nview_rc_s_g.extents(),
                 "size:",    nview_rc_s_g.size());

  EXPECT_EQ_U(2,             nview_rows_g.extent<0>());
  EXPECT_EQ_U(mat.extent(1), nview_rows_g.extent<1>());

  EXPECT_EQ_U(nview_rc_s_g.extents(), nview_cr_s_g.extents());
  EXPECT_EQ_U(nview_rc_s_g.offsets(), nview_cr_s_g.offsets());

#if __TODO__
  auto nview_rows_l = dash::local(nview_rows_g);

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimLocalView",
                     nview_rows_l.extents());

  EXPECT_EQ_U(2,             nview_rows_l.extent<0>());
  EXPECT_EQ_U(block_cols,    nview_rows_l.extent<1>());
#endif
}

