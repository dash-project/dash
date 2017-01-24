
#include "NViewTest.h"

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <array>


TEST_F(NViewTest, MatrixBlocked1DimLocalView)
{
  auto nunits = dash::size();

  int block_rows = 2;
  int block_cols = 3;

  int nrows = nunits * block_rows;
  int ncols = nunits * block_cols;

  dash::Matrix<int, 2> mat(
      nrows,         ncols,
      dash::BLOCKED, dash::BLOCKED);

  auto nview_rows_0_2 = dash::sub<0>(0, 2, mat);
}

