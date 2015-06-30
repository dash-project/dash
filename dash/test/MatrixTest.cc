#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MatrixTest.h"

TEST_F(MatrixTest, SingleWriteMultipleRead) {
  dart_unit_t myid  = dash::myid();
  size_t extent_cols = 431;
  size_t extent_rows = 547;
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
        matrix[i][k] = (i * 11) + (k * 97);
      }
    }
  }
  // Units waiting for value initialization
  dash::Team::All().barrier();

  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(0); ++i) {
    for(int k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

