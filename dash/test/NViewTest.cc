
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
  auto nview_rows_g = dash::sub<0>(0, 2, mat);

  EXPECT_EQ_U(2,             nview_rows_g.extent<0>());
  EXPECT_EQ_U(mat.extent(1), nview_rows_g.extent<1>());

  auto nview_rows_l = dash::local(nview_rows_g);

  EXPECT_EQ_U(2,             nview_rows_l.extent<0>());
//EXPECT_EQ_U(block_cols,    nview_rows_l.extent<1>());
}

