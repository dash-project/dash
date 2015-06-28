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
  EXPECT_EQ(matrix_size, matrix.size());
  // Create array instances using varying constructor options
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning array values");
    for(int i = 0; i < matrix.extent(i); ++i) {
      for(int k = 0; k < matrix.extent(k); ++k) {
        matrix[i][k] = i + k;
      }
    }
  }
  // Units waiting for value initialization
  dash::Team::All().barrier();
  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(i); ++i) {
    for(int k = 0; k < matrix.extent(k); ++k) {
      int value = matrix[i][k];
      ASSERT_EQ_U(i + k, value);
    }
  }
}

