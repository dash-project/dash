#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MatrixTest.h"

TEST_F(MatrixTest, SingleWriteMultipleRead) {
  dart_unit_t myid  = dash::myid();
  size_t extent_cols = 19;
  size_t extent_rows = 41;
  dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           extent_cols, extent_rows),
                         dash::DistributionSpec<2>(
                           dash::BLOCKED, dash::NONE));
  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  // Create array instances using varying constructor options
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        LOG_MESSAGE("Matrix[i]");
        dash::MatrixRef<int, 2, 1> matrix_ref_i  = matrix[i];
        LOG_MESSAGE("Matrix[i][k]");
        dash::MatrixRef<int, 2, 0> matrix_ref_ik = matrix_ref_i[k];
        LOG_MESSAGE("Assigning (%d,%d) = %d", i, k, i + k);
        matrix_ref_ik = i + k;
      }
    }
  }
  // Units waiting for value initialization
  dash::Team::All().barrier();
  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(i); ++i) {
    for(int k = 0; k < matrix.extent(k); ++k) {
      int value    = matrix[i][k];
      int expected = i + k;
      LOG_MESSAGE("Checking i:%d k:%d", i, k);
      ASSERT_EQ_U(expected, value);
    }
  }
}

