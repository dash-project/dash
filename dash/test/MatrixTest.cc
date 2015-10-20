#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MatrixTest.h"

TEST_F(MatrixTest, SingleWriteMultipleRead)
{
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

TEST_F(MatrixTest, Distribute1DimBlockcyclicY)
{
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t extent_cols = 43;
  size_t extent_rows = 54;
  typedef dash::Pattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2, pattern_t::index_type, pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::NONE,
                   dash::BLOCKCYCLIC(5)),
                 dash::Team::All(),
                 team_spec);
  
  LOG_MESSAGE("Matrix initialized, wait for barrier ...");
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
  LOG_MESSAGE("Values assigned, wait for barrier ...");
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

TEST_F(MatrixTest, Distribute2DimTileXY)
{
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  typedef dash::TilePattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2, pattern_t::index_type, pattern_t> matrix(
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

TEST_F(MatrixTest, Distribute2DimBlockcyclicXY)
{
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t blocksize_x = 3;
  size_t blocksize_y = 2;
  size_t extent_cols = blocksize_x * num_units * 2;
  size_t extent_rows = blocksize_y * num_units * 2;
  typedef dash::Pattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  EXPECT_EQ_U(team_spec.size(), num_units);
  EXPECT_EQ_U(team_spec.rank(), 1);
  dash::Matrix<int, 2, pattern_t::index_type, pattern_t> matrix(
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

TEST_F(MatrixTest, Submat2DimDefault)
{
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  typedef dash::Pattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<int, 2, pattern_t::index_type, pattern_t> matrix(
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

  auto matrix_size = matrix.size();
  ASSERT_EQ_U(extent_cols * extent_rows, matrix_size);
  
  // Columns 0 ... (J/2)
  LOG_MESSAGE("Testing sub<0>(0, J/2)");
  auto submatrix_x_lower = matrix.sub<0>(0,
                                            extent_cols / 2);
  ASSERT_EQ_U(matrix_size/2, submatrix_x_lower.size());
  // Columns (J/2) ... (J-1)
  LOG_MESSAGE("Testing sub<0>(J/2, J-1)");
  auto submatrix_x_upper = matrix.sub<0>(extent_cols / 2,
                                            extent_cols / 2);
  ASSERT_EQ_U(matrix_size/2, submatrix_x_upper.size());
  // Rows 0 ... (J/2)
  LOG_MESSAGE("Testing sub<1>(0, I/2)");
  auto submatrix_y_lower = matrix.sub<1>(0,
                                            extent_rows / 2);
  ASSERT_EQ_U(matrix_size/2, submatrix_y_lower.size());
  // Rows (J/2) ... (J-1)
  LOG_MESSAGE("Testing sub<1>(I/2, I-1)");
  auto submatrix_y_upper = matrix.sub<1>(extent_rows / 2,
                                            extent_rows / 2);
  ASSERT_EQ_U(matrix_size/2, submatrix_y_upper.size());
}

TEST_F(MatrixTest, Sub2DimDefault)
{
  typedef int element_t;
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  typedef dash::TilePattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t> matrix(
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

  auto matrix_size = matrix.size();
  // Check matrix size:
  ASSERT_EQ_U(extent_cols * extent_rows, matrix_size);
  // Plausibility checks of matrix pattern:
  const pattern_t & pattern = matrix.pattern();
  ASSERT_EQ_U(matrix_size, pattern.size());
  ASSERT_EQ_U(matrix.local_size(), pattern.local_size());
  ASSERT_EQ_U(matrix.local_capacity(), pattern.local_capacity());
  // Check local range:
  ASSERT_EQ_U(matrix_size / num_units, matrix.local_capacity());
  ASSERT_EQ_U(matrix_size / num_units, matrix.local_size());
  element_t * lit  = matrix.lbegin();
  element_t * lend = matrix.lend();
  LOG_MESSAGE("Local range: lend(%p) - lbegin(%p) = %d",
              lend, lit, lend - lit);
  ASSERT_EQ_U(matrix.lend() - matrix.lbegin(),
              matrix.local_size());
  // Assign unit-specific values in local matrix range:
  for (int lidx = 0; lit != lend; ++lidx, ++lit) {
    ASSERT_LT_U(lidx, matrix.local_size());
    auto value = ((dash::myid() + 1) * 1000) + lidx;
    LOG_MESSAGE("Assigning local address (%p) %d = %d",
                lit, lidx, value);
    *lit = value;
  }
  
  matrix.barrier();
  LOG_MESSAGE("Testing values");

  // Test values by column:
  size_t num_visited_total = 0;
  size_t num_visited_local = 0;
  for (auto col = 0; col < extent_cols; ++col) {
    auto column = matrix.sub<0>(col);
    for (auto row = 0; row < extent_rows; ++row) {
      auto g_coords   = std::array<int, 2> { col, row };
      auto l_coords   = pattern.local_coords(g_coords);
      auto unit_id    = pattern.unit_at(g_coords);
      auto local_idx  = pattern.local_at(l_coords);
      auto global_idx = pattern.memory_layout().at(g_coords);
      auto exp_value  = ((unit_id + 1) * 1000) + local_idx;
      bool is_local   = unit_id == dash::myid();
      LOG_MESSAGE("Testing g(%d,%d) l(%d,%d) u(%d) li(%d) == %d",
                  col, row,
                  l_coords[0], l_coords[1],
                  unit_id,
                  local_idx,
                  exp_value);
      element_t value = column[row];
      ASSERT_EQ_U(exp_value, value);
      ASSERT_EQ_U(is_local, matrix.is_local(global_idx));
      if (is_local) {
        ++num_visited_local;
      }
      ++num_visited_total;
    }
  }
  // Check number of iterated local and total elements:
  ASSERT_EQ_U(matrix_size, num_visited_total);
  ASSERT_EQ_U(matrix.local_size(), num_visited_local);
}

TEST_F(MatrixTest, BlockViews)
{
  typedef int element_t;
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t tilesize    = tilesize_x * tilesize_y;
  size_t extent_cols = tilesize_x * num_units * 4;
  size_t extent_rows = tilesize_y * num_units * 4;
  typedef dash::TilePattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_x),
                   dash::TILE(tilesize_y)),
                 dash::Team::All(),
                 team_spec);
  // Fill matrix
  if(myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int col = 0; col < matrix.extent(0); ++col) {
      for(int row = 0; row < matrix.extent(1); ++row) {
        auto value = (row * matrix.extent(0)) + col;
        LOG_MESSAGE("Setting matrix[%d][%d] = %d",
                    col, row, value);
        matrix[col][row] = value;
      }
    }
  }
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  element_t exp_val;

  // View at block at global block offset 0 (first global block):
  auto block_gi_0 = matrix.block(0);
  ASSERT_EQ_U(tilesize, block_gi_0.size());

  // Test first element in block at global block index 0:
  exp_val = matrix[0][0];
  ASSERT_EQ_U(exp_val,
              *(block_gi_0.begin()));
  // Test last element in block at global block index 0:
  exp_val = matrix[tilesize_x-1][tilesize_y-1];
  ASSERT_EQ_U(exp_val, 
              *(block_gi_0.begin() + (tilesize-1)));

  // View at block at global block offset 6
  // (first global block of lower right matrix quarter):
  auto nblocks_x  = matrix.extents()[0] / tilesize_x;
  auto nblocks_y  = matrix.extents()[1] / tilesize_y;
  // Block index of first block in lower right quarter of the matrix:
  auto block_q_gi = ((nblocks_x * nblocks_y) / 2) + (nblocks_x / 2);
  auto block_gi_q = matrix.block(block_q_gi);
  ASSERT_EQ_U(tilesize, block_gi_q.size());

  // Test first element in first block at lower right quarter of the matrix:
  auto block_6_x = matrix.extents()[0] / 2;
  auto block_6_y = matrix.extents()[1] / 2;
  exp_val = matrix[block_6_x][block_6_y];
  ASSERT_EQ_U(exp_val,
              *(block_gi_q.begin()));
  // Test last element in first block at lower right quarter of the matrix:
  exp_val = matrix[block_6_x+tilesize_x-1][block_6_y+tilesize_y-1];
  ASSERT_EQ_U(exp_val, 
              *(block_gi_q.begin() + (tilesize-1)));
}

TEST_F(MatrixTest, SubBlockIteration)
{
  typedef int element_t;
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 4;
  size_t extent_rows = tilesize_y * num_units * 4;
  typedef dash::TilePattern<2> pattern_t;
  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t> matrix(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_x),
                   dash::TILE(tilesize_y)),
                 dash::Team::All(),
                 team_spec);
  // Fill matrix
  if(myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 1000) + (k * 1);
        LOG_MESSAGE("Setting matrix[%d][%d] = %d",
                    i, k, value);
        matrix[i][k] = value;
      }
    }
  }
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Partition matrix into 4 blocks (upper/lower left/right):
  
  // First create two views for left and right half:
  auto left        = matrix.sub<0>(0,               extent_cols / 2);
  auto right       = matrix.sub<0>(extent_cols / 2, extent_cols / 2);

  // Refine views on left and right half into top/bottom:
  auto topleft     = left.sub<1>(0,               extent_rows / 2);
  auto bottomleft  = left.sub<1>(extent_rows / 2, extent_rows / 2);
  auto topright    = right.sub<1>(0,               extent_rows / 2);
  auto bottomright = right.sub<1>(extent_rows / 2, extent_rows / 2);

  auto g_br_x      = extent_cols / 2;
  auto g_br_y      = extent_rows / 2;

  // Initial plausibility check: Access same element by global- and view
  // coordinates:
  ASSERT_EQ_U((int)bottomright[0][0],
              (int)matrix[g_br_x][g_br_y]);

  int phase              = 0;
  // Extents of the view projection:
  int view_size_x        = extent_cols / 2;
  int view_size_y        = extent_rows / 2;
  int view_size          = view_size_x * view_size_y;
  // Global coordinates of first element in bottom right block:
  int block_base_coord_x = extent_cols / 2;
  int block_base_coord_y = extent_rows / 2;
  auto b_it  = bottomright.begin();
  auto b_end = bottomright.end();
  for (; b_it != b_end; ++b_it, ++phase) {
    int phase_x  = phase % view_size_x;
    int phase_y  = phase / view_size_x;
    int gcoord_x = block_base_coord_x + phase_x;
    int gcoord_y = block_base_coord_y + phase_y;
    LOG_MESSAGE("phase:%d = %d,%d pos:%d gcoord:%d,%d",
                phase,
                phase_x, phase_y,
                b_it.pos(),
                gcoord_x, gcoord_y);
    ASSERT_EQ_U(phase, b_it.pos());
    // Apply view projection by converting to GlobPtr:
    dash::GlobPtr<int> block_elem_gptr = (dash::GlobPtr<int>)(b_it);
    // Compare with GlobPtr from global iterator without view projection:
    dash::GlobPtr<int> glob_elem_gptr  = (dash::GlobPtr<int>)(
                                          matrix[gcoord_x][gcoord_y]);
    int block_value = *block_elem_gptr;
    int glob_value  = *glob_elem_gptr;
    ASSERT_EQ_U(glob_value,
                block_value);
    ASSERT_EQ_U(glob_elem_gptr, block_elem_gptr);
  }
}

