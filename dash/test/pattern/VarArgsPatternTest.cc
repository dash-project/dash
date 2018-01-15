#include "VarArgsPatternTest.h"
#include "../TestBase.h"

#include <gtest/gtest.h>

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/TeamSpec.h>
#include <dash/Matrix.h>

#define NX  10
#define BCX 5
#define NY  6
#define BCY 3


TEST_F(VarArgsPatternTest, SimpleConstructorTest)
{
  size_t size = dash::size();
  //  Matrix uses TilePattern by default
  {
    dash::Matrix<int, 2> mat(size*NX, size*NY);
    ASSERT_GT_U(mat.local_size(), 0);
  }
  {
    dash::Matrix<int, 2> mat(size*NX, size*NY,
                             dash::BLOCKED, dash::BLOCKCYCLIC(BCY));
    ASSERT_GT_U(mat.local_size(), 0);
  }
  {
    dash::Matrix<int, 2> mat(size*NX, size*NY,
                             dash::BLOCKCYCLIC(BCX), dash::BLOCKCYCLIC(BCY));
    ASSERT_GT_U(mat.local_size(), 0);
  }

  //  NArray uses BlockPattern by default
  {
    dash::NArray<int, 2> mat(size*NX, size*NY,
                             dash::BLOCKCYCLIC(BCX), dash::BLOCKCYCLIC(BCY));
    ASSERT_GT_U(mat.local_size(), 0);
  }

  //  shift tile pattern
  {
    dash::Matrix<int, 2, dash::default_index_t,
                 dash::ShiftTilePattern<2> > mat(size*NX, size*NY,
                                                 dash::TILE(BCX), dash::TILE(BCY));
    ASSERT_GT_U(mat.local_size(), 0);
  }
}
