#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MatrixTest.h"

TEST_F(MatrixTest, SingleWriteMultipleRead) {
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 7;
  size_t tilesize_y  = 3;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           extent_cols,
                           extent_rows),
                         dash::DistributionSpec<2>(
                           dash::TILE(tilesize_x),
                           dash::TILE(tilesize_y)));
  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
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

TEST_F(MatrixTest, Distribute1DimBlockcyclicY) {
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t extent_cols = 43;
  size_t extent_rows = 54;
  typedef dash::Pattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2,
               pattern_t::index_type, 
               pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::NONE,
                   dash::BLOCKCYCLIC(5)),
                 dash::Team::All(),
                 team_spec);
  
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        LOG_MESSAGE("Setting matrix[%d][%d] = %d",
                    i, k, value);
        matrix[i][k] = value;
      }
    }
  }
  // Units waiting for value initialization
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(0); ++i) {
    for(int k = 0; k < matrix.extent(1); ++k) {
      LOG_MESSAGE("Testing matrix[%d][%d]", i, k);
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Distribute2DimTileXY) {
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  typedef dash::TilePattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2,
               pattern_t::index_type,
               pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_x),
                   dash::TILE(tilesize_y)),
                 dash::Team::All(),
                 team_spec);
  
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if(myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        LOG_MESSAGE("Setting matrix[%d][%d] = %d",
                    i, k, value);
        matrix[i][k] = value;
      }
    }
  }

  // Units waiting for value initialization
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(0); ++i) {
    for(int k = 0; k < matrix.extent(1); ++k) {
      LOG_MESSAGE("Testing matrix[%d][%d]", i, k);
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Distribute2DimBlockcyclicXY) {
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t blocksize_x = 3;
  size_t blocksize_y = 2;
  size_t extent_cols = blocksize_x * num_units * 2;
  size_t extent_rows = blocksize_y * num_units * 2;
  typedef dash::Pattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2,
               pattern_t::index_type,
               pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::BLOCKCYCLIC(blocksize_x),
                   dash::BLOCKCYCLIC(blocksize_y)),
                 dash::Team::All(),
                 team_spec);
  
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if(myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        LOG_MESSAGE("Setting matrix[%d][%d] = %d",
                    i, k, value);
        matrix[i][k] = value;
      }
    }
  }
  // Units waiting for value initialization
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(0); ++i) {
    for(int k = 0; k < matrix.extent(1); ++k) {
      LOG_MESSAGE("Testing matrix[%d][%d]", i, k);
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

