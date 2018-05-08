
#include "MatrixTest.h"

#include <dash/Matrix.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Distribution.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Generate.h>

#include <dash/pattern/internal/PatternLogging.h>

#include <iostream>
#include <iomanip>


TEST_F(MatrixTest, OddSize)
{
  typedef dash::Pattern<2>                 pattern_t;
  typedef typename pattern_t::index_type   index_t;

  dash::Matrix<int, 2, index_t, pattern_t> matrix(dash::SizeSpec<2>(8, 15));

  for (size_t i = 0; i < matrix.extent(0); i++) {
    for (size_t j = 0; j < matrix.extent(1); j++) {
      if (matrix(i,j).is_local()) {
        DASH_LOG_TRACE("MatrixText.OddSize", "(", i, ",", j, ")",
                       "unit:", dash::myid().id);
      }
    }
  }
}

TEST_F(MatrixTest, LocalAccess)
{
  const int n_brow = 4;
  const int n_bcol = 3;

  auto myid = dash::myid();

  dash::NArray<int, 2> mat(n_brow * dash::size(),
                           n_bcol * dash::size());

  DASH_LOG_DEBUG("MatrixTest.ElementAccess",
                 "matrix extents:", mat.extent(0), "x", mat.extent(1));
  DASH_LOG_DEBUG("MatrixTest.ElementAccess",
                 "matrix local view:", mat.local.extents());

  int lcount = (myid + 1) * 1000;
  dash::generate(mat.begin(), mat.end(), 
                 [&]() {
                   return (lcount++);
                 });
  mat.barrier();

  DASH_LOG_DEBUG("MatrixTest.ElementAccess", "Matrix initialized");

  for (int i = 0; i < mat.local.extent(0); i++) {
    for (int j = 0; j < mat.local.extent(1); j++) {
      DASH_LOG_DEBUG("MatrixTest.ElementAccess",
                     "mat.local[", i, "][", j, "]");
      EXPECT_EQ(mat.local(i,j),
                mat.local[i][j]);
    }
  }
}

TEST_F(MatrixTest, Views)
{
  const size_t block_size_x  = 3;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t num_local_blocks_x  = 3;
  size_t num_local_blocks_y  = 2;
  size_t num_blocks_x        = dash::size() * num_local_blocks_x;
  size_t num_blocks_y        = dash::size() * num_local_blocks_y;
  size_t num_blocks_total    = num_blocks_x * num_blocks_y;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / dash::size();
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;

  LOG_MESSAGE("nunits:%lu elem_total:%lu elem_per_unit:%lu "
              "blocks_per_unit:%lu",
              dash::size(), num_elem_total,
              num_elem_per_unit, num_blocks_per_unit);

  typedef dash::default_index_t                 index_t;
  typedef dash::TilePattern<2, dash::COL_MAJOR> pattern_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_x,
      extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y))
  );

  dash::Matrix<int, 2, index_t, pattern_t> matrix(pattern);

  // Test viewspecs of blocks in global index domain:
  if (dash::myid() == 0) {
    LOG_MESSAGE("Testing viewspecs of blocks in global index domain");
    for (size_t b = 0; b < num_blocks_total; ++b) {
      DASH_LOG_TRACE("MatrixTest.Views", "Testing viewspec of block", b);
      auto g_block       = matrix.block(b);
      auto g_block_first = g_block.begin();
      auto g_block_view  = g_block_first.viewspec();
      DASH_LOG_TRACE("MatrixTest.Views", "block viewspec:",
                     "offset: (", g_block_view.offset(0), ",",
                                  g_block_view.offset(1), ")",
                     "extent: (", g_block_view.extent(0), ",",
                                  g_block_view.extent(1), ")");
      // Global block coordinates:
      auto g_block_x     = b % num_blocks_x;
      auto g_block_y     = b / num_blocks_x;
      // Global coordinates of first block element:
      auto g_elem_x      = g_block_x * block_size_x;
      auto g_elem_y      = g_block_y * block_size_y;
      ASSERT_EQ_U(g_elem_x, g_block_view.offset(0));
      ASSERT_EQ_U(g_elem_y, g_block_view.offset(1));
      // Extent (block_size_x, block_size_y):
      ASSERT_EQ_U(block_size_x, g_block_view.extent(0));
      ASSERT_EQ_U(block_size_y, g_block_view.extent(1));
    }
  }

  // To improve readability of log output:
  dash::barrier();

  // Test viewspecs of blocks in local index domain:
  LOG_MESSAGE("Testing viewspecs of blocks in local index domain");
  int lb = 0;
  for (size_t b = 0; b < num_blocks_total; ++b) {
    auto g_block       = matrix.block(b);
    auto g_block_first = g_block.begin();
    auto g_block_view  = g_block_first.viewspec();
    LOG_MESSAGE("Checking if block %lu is local", b);
    if (g_block_first.is_local()) {
      LOG_MESSAGE("Testing viewspec of local block %d", lb);
      auto l_block       = matrix.local.block(lb);
      auto l_block_first = l_block.begin();
      auto l_block_view  = l_block_first.viewspec();
      DASH_LOG_TRACE("MatrixTest.Views", "global block viewspec:",
                     "offset: (", g_block_view.offset(0), ",",
                                  g_block_view.offset(1), ")",
                     "extent: (", g_block_view.extent(0), ",",
                                  g_block_view.extent(1), ")");
      DASH_LOG_TRACE("MatrixTest.Views", "local block viewspec:",
                     "offset: (", l_block_view.offset(0), ",",
                                  l_block_view.offset(1), ")",
                     "extent: (", l_block_view.extent(0), ",",
                                  l_block_view.extent(1), ")");
      // Verify matrix.block(b) == matrix.local.block(lb):
      ASSERT_EQ_U(g_block_view, l_block_view);
      ++lb;
    }
  }
  // Validate number of local blocks found:
  ASSERT_EQ_U(num_blocks_per_unit, lb);
}

TEST_F(MatrixTest, SingleWriteMultipleRead)
{
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
  LOG_MESSAGE("Matrix size: %lu", matrix_size);
  // Fill matrix
  if(dash::myid().id == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        matrix[i][k] = (i * 11) + (k * 97);
      }
    }
  }
  // Units waiting for value initialization
  dash::Team::All().barrier();

  // Read and assert values in matrix
  for(size_t i = 0; i < matrix.extent(0); ++i) {
    for(size_t k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Distribute1DimBlockcyclicY)
{
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
  matrix.barrier();
  LOG_MESSAGE("Team barrier passed");

  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %lu", matrix_size);
  // Fill matrix
  if(dash::myid().id == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        matrix[i][k] = value;
      }
    }
  }
  // Units waiting for value initialization
  LOG_MESSAGE("Values assigned, wait for barrier ...");
  matrix.barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(size_t i = 0; i < matrix.extent(0); ++i) {
    for(size_t k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Distribute2DimTileXY)
{
  dash::global_unit_t myid = dash::myid();
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
                   extent_rows,
                   extent_cols),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_y),
                   dash::TILE(tilesize_x)),
                 dash::Team::All(),
                 team_spec);

  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_rows, matrix.extent(0));
  ASSERT_EQ(extent_cols, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %lu", matrix_size);
  // Fill matrix
  if (myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        matrix[i][k] = value;
      }
    }
  }

  // Units waiting for value initialization
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(size_t i = 0; i < matrix.extent(0); ++i) {
    for(size_t k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Distribute2DimBlockcyclicXY)
{
  dash::global_unit_t myid = dash::myid();
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
  LOG_MESSAGE("Matrix size: %lu", matrix_size);
  // Fill matrix
  if (myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        auto value = (i * 11) + (k * 97);
        matrix[i][k] = value;
      }
    }
  }
  // Units waiting for value initialization
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  // Read and assert values in matrix
  for(size_t i = 0; i < matrix.extent(0); ++i) {
    for(size_t k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      ASSERT_EQ_U(expected, value);
    }
  }
}

TEST_F(MatrixTest, Submat2DimDefault)
{
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
  typedef dash::default_index_t index_t;
  typedef int                   element_t;
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
  LOG_MESSAGE("Local range: lend(%p) - lbegin(%p) = %ld",
              static_cast<void*>(lend), static_cast<void*>(lit),
              lend - lit);
  ASSERT_EQ_U(matrix.lend() - matrix.lbegin(),
              matrix.local_size());
  // Assign unit-specific values in local matrix range:
  for (index_t lidx = 0; lit != lend; ++lidx, ++lit) {
    ASSERT_LT_U(lidx, matrix.local_size());
    auto value = ((dash::myid() + 1) * 1000) + lidx;
    *lit = value;
  }

  matrix.barrier();
  LOG_MESSAGE("Testing values");

  // Test values by column:
  size_t num_visited_total = 0;
  size_t num_visited_local = 0;
  for (index_t col = 0; col < static_cast<index_t>(extent_cols); ++col) {
    auto column = matrix.sub<0>(col);
    for (index_t row = 0; row < static_cast<index_t>(extent_rows); ++row) {
      auto g_coords   = std::array<index_t, 2> {{ col, row }};
      auto l_coords   = pattern.local_coords(g_coords);
      auto unit_id    = pattern.unit_at(g_coords);
      auto local_idx  = pattern.local_at(l_coords);
      auto global_idx = pattern.memory_layout().at(g_coords);
      auto exp_value  = ((unit_id + 1) * 1000) + local_idx;
      bool is_local   = unit_id == pattern.team().myid();
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
  int    myid        = dash::myid().id;
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
  if (myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t col = 0; col < matrix.extent(0); ++col) {
      for(size_t row = 0; row < matrix.extent(1); ++row) {
        auto value = (row * matrix.extent(0)) + col;
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

TEST_F(MatrixTest, ViewIteration)
{
  typedef int                                   element_t;
  typedef dash::TilePattern<2, dash::ROW_MAJOR> pattern_t;

  int    myid        = dash::myid().id;
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t ncols       = tilesize_x * num_units * 2;
  size_t nrows       = tilesize_y * num_units * 2;

  LOG_MESSAGE("Initialize matrix ...");
  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t> matrix(
                 dash::SizeSpec<2>(
                   nrows,
                   ncols),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_y),
                   dash::TILE(tilesize_x)),
                 dash::Team::All(),
                 team_spec);
  // Fill matrix
  if (myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        auto value = ((i + 1) * 1000) + (k * 1);
        matrix[i][k] = value;
      }
    }
  }
  dash::Team::All().barrier();

  if (myid == 0) {
    // Partition matrix into 4 blocks (upper/lower left/right):

    // First create two views for left and right half:
    auto left        = matrix.sub<1>(0,         ncols / 2);
    auto right       = matrix.sub<1>(ncols / 2, ncols / 2);

    // Refine views on left and right half into top/bottom:
    auto topleft     = left.sub<0> (0,         nrows / 2);
    auto bottomleft  = left.sub<0> (nrows / 2, nrows / 2);
    auto topright    = right.sub<0>(0,         nrows / 2);
    auto bottomright = right.sub<0>(nrows / 2, nrows / 2);

    dash__unused(topleft);
    dash__unused(bottomleft);
    dash__unused(topright);

    auto g_br_x      = ncols / 2;
    auto g_br_y      = nrows / 2;

    // Initial plausibility check: Access same element by global- and view
    // coordinates:
    ASSERT_EQ_U((int)bottomright[0][0],
                (int)matrix[g_br_y][g_br_x]);

    dash::test::print_matrix("Matrix<2>", matrix, 3);

    for (int i = 0; i < bottomright.extent(0); ++i) {
      DASH_LOG_DEBUG_VAR("MatrixTest.ViewIteration",
                         bottomright[i].viewspec());
      std::vector<int> row(bottomright[i].begin(),
                           bottomright[i].end());
      DASH_LOG_DEBUG("MatrixTest.ViewIteration",
                     "bottomright[",i,"]", row);
    }

    int  phase              = 0;
    // Extents of the view projection:
    int  view_size_x        = ncols / 2;
    // Global coordinates of first element in bottom right block:
    int  block_base_coord_x = ncols / 2;
    int  block_base_coord_y = nrows / 2;
    auto b_it               = bottomright.begin();
    auto b_end              = bottomright.end();
    int  block_index_offset = b_it.pos();
    for (; b_it != b_end; ++b_it, ++phase) {
      int phase_x  = phase % view_size_x;
      int phase_y  = phase / view_size_x;
      int gcoord_x = block_base_coord_x + phase_x;
      int gcoord_y = block_base_coord_y + phase_y;
      ASSERT_EQ_U(phase, (b_it.pos() - block_index_offset));

      using glob_it_t   = decltype(matrix.begin());
      using glob_ptr_t  = typename glob_it_t::pointer;
      using glob_cptr_t = dash::GlobConstPtr<int>;

      // Apply view projection by converting to GlobPtr:
      glob_ptr_t  block_elem_gptr = static_cast<glob_ptr_t>(b_it);
      // Compare with GlobPtr from global iterator without view projection:
      glob_cptr_t glob_elem_gptr(matrix[gcoord_y][gcoord_x].dart_gptr());
      int block_value = *block_elem_gptr;
      int glob_value  = *glob_elem_gptr;

      if (glob_value != block_value) {
        DASH_LOG_DEBUG("MatrixTest.ViewIteration",
                       "gcoords:(", gcoord_y, ",", gcoord_x, ")",
                       "vcoords:(", phase_y,  ",", phase_x,  ")",
                       "v.phase:",  phase);
        DASH_LOG_DEBUG("MatrixTest.ViewIteration",
                       "it:",       dash::typestr(b_it),
                       "it.pos:",   b_it.pos(),
                       "it.gpos:",  b_it.gpos());
        DASH_LOG_DEBUG("MatrixTest.ViewIteration",
                       "view.gptr:", block_elem_gptr,
                       "glob.gptr:", glob_elem_gptr);
      }
      ASSERT_EQ_U(glob_value,     block_value);
      ASSERT_EQ_U(glob_elem_gptr, block_elem_gptr);
    }
  }
}

TEST_F(MatrixTest, BlockCopy)
{
  typedef int element_t;
  int    myid = dash::myid().id;
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 2;
  size_t extent_cols = tilesize_x * num_units * 4;
  size_t extent_rows = tilesize_y * num_units * 4;
  typedef dash::TilePattern<2> pattern_t;

  dash::TeamSpec<2> team_spec(num_units, 1);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t>
               matrix_a(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_x),
                   dash::TILE(tilesize_y)),
                 dash::Team::All(),
                 team_spec);
  dash::Matrix<element_t, 2, pattern_t::index_type, pattern_t>
               matrix_b(
                 dash::SizeSpec<2>(
                   extent_cols,
                   extent_rows),
                 dash::DistributionSpec<2>(
                   dash::TILE(tilesize_x),
                   dash::TILE(tilesize_y)),
                 dash::Team::All(),
                 team_spec);
  // Fill matrix
  if (myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t col = 0; col < matrix_a.extent(0); ++col) {
      for(size_t row = 0; row < matrix_a.extent(1); ++row) {
        auto value = (row * matrix_a.extent(0)) + col;
        matrix_a[col][row] = value;
        matrix_b[col][row] = value;
      }
    }
  }
  LOG_MESSAGE("Wait for team barrier ...");
  dash::barrier();
  LOG_MESSAGE("Team barrier passed");

  // Copy block 1 of matrix_a to block 0 of matrix_b:
  dash::copy<element_t>(matrix_a.block(1).begin(),
                        matrix_a.block(1).end(),
                        matrix_b.block(0).begin());

  LOG_MESSAGE("Wait for team barrier ...");
  dash::barrier();
  LOG_MESSAGE("Team barrier passed");
}

TEST_F(MatrixTest, StorageOrder)
{
  size_t num_units = dash::size();

  size_t tilesize_row = 5;
  size_t tilesize_col = 4;
  size_t nrows = tilesize_row * num_units * 2;
  size_t ncols = tilesize_col * num_units * 2;

  dash::TilePattern<2, dash::ROW_MAJOR> pat_row(
    nrows, ncols, dash::TILE(tilesize_row), dash::TILE(tilesize_col));
  dash::TilePattern<2, dash::COL_MAJOR> pat_col(
    nrows, ncols, dash::TILE(tilesize_row), dash::TILE(tilesize_col));

  typedef dash::default_index_t index_t;

  if (dash::myid().id == 0) {
    dash::test::print_pattern_mapping(
      "pattern.row-major.local_index", pat_row, 3,
      [](const decltype(pat_row) & _pattern, int _x, int _y) -> index_t {
          return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
      });
    dash::test::print_pattern_mapping(
      "pattern.col-major.local_index", pat_col, 3,
      [](const decltype(pat_col) & _pattern, int _x, int _y) -> index_t {
          return _pattern.local_index(std::array<index_t, 2> {_x, _y}).index;
      });
  }

  dash::Matrix<int, 2, index_t, decltype(pat_col)> mat_col(pat_col);
  dash::Matrix<int, 2, index_t, decltype(pat_row)> mat_row(pat_row);

  ASSERT_EQ_U(mat_row.local_size(), mat_row.local.size());
  ASSERT_GT_U(mat_row.local.size(), 0);
  ASSERT_EQ_U(mat_col.local_size(), mat_col.local.size());
  ASSERT_GT_U(mat_col.local.size(), 0);

  for (int i = 0; i < static_cast<int>(mat_row.local.size()); ++i) {
     mat_row.lbegin()[i] = 1000 * (dash::myid().id + 1) + i;
     mat_col.lbegin()[i] = 1000 * (dash::myid().id + 1) + i;
  }

  dash::barrier();
}

TEST_F(MatrixTest, DelayedAlloc)
{
  dash::team_unit_t myid(dash::myid());
  auto num_units   = dash::size();

  if (num_units < 4) {
    LOG_MESSAGE("MatrixTest.DelayedAlloc requires at least 4 units");
    return;
  }

  // Default constructor creates team spec with extents (nunits, 1, 1):
  dash::TeamSpec<3> teamspec;
  // Automatic balancing of team spec in three dimensions:
  teamspec.balance_extents();

  // reverse team extents
  auto team_extents = teamspec.extents();
  if (team_extents[0] > team_extents[2]) {
    std::swap(team_extents[0], team_extents[2]);
    teamspec.resize(team_extents);
  }

  if (myid == 0) {
    DASH_LOG_TRACE_VAR("MatrixTest.DelayedAlloc", teamspec.extents());
  }

  auto num_units_i = teamspec.extent(0);
  auto num_units_j = teamspec.extent(1);
  auto num_units_k = teamspec.extent(2);

  // Cartesian dimensions for row-major storage order:
  // index (i,j,k) = Cartesian offset (z,y,x)
  auto tilesize_i   = 2;
  auto tilesize_j   = 5;
  auto tilesize_k   = 3;
  auto blocksize    = tilesize_i * tilesize_j * tilesize_k;
  auto num_blocks_i = num_units_i > 1 ? 2 * num_units_i : 1;
  auto num_blocks_j = num_units_j > 1 ? 3 * num_units_j : 1;
  auto num_blocks_k = num_units_k > 1 ? 2 * num_units_k : 1;
  auto extent_i     = num_blocks_i * tilesize_i;
  auto extent_j     = num_blocks_j * tilesize_j;
  auto extent_k     = num_blocks_k * tilesize_k;

  typedef double
    value_t;
  typedef dash::default_index_t
    index_t;
  typedef dash::default_extent_t
    extent_t;
  typedef dash::CartesianIndexSpace<3, dash::ROW_MAJOR, index_t>
    index_space_t;

  dash::barrier();
  DASH_LOG_DEBUG("MatrixTest.DelayedAlloc",
                 "Calling dash::Matrix default constructor");

  dash::Matrix<
          value_t,
          3,
          index_t,
          dash::TilePattern<3> > mx;

  ASSERT_EQ_U(num_units, teamspec.size());

  dash::barrier();
  DASH_LOG_DEBUG("MatrixTest.DelayedAlloc",
                 "Calling dash::Matrix.allocate");

  // Delayed allocation of matrix:
  mx.allocate(
      dash::SizeSpec<3>(
        extent_i,
        extent_j,
        extent_k),
      dash::DistributionSpec<3>(
        dash::TILE(tilesize_i),
        dash::TILE(tilesize_j),
        dash::TILE(tilesize_k)),
      teamspec
  );

  auto pattern        = mx.pattern();
  auto blockspec      = pattern.blockspec().extents();
  auto blocksizespec  = pattern.block(0).extents();
  auto n_local_blocks = pattern.local_blockspec().size();
  auto n_local_elem   = n_local_blocks * blocksize;

  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", blockspec);
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", blocksizespec);
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", blocksize);
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", mx.local.extents());
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", mx.local.offsets());
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", n_local_blocks);
  DASH_LOG_DEBUG_VAR("MatrixTest.DelayedAlloc", n_local_elem);

  ASSERT_EQ_U(mx.local.size(), n_local_elem);

  // Initialize values:
  for (extent_t lbi = 0; lbi < n_local_blocks; ++lbi) {
    // submatrix view on local block obtained from matrix relative to global
    // memory space:
    auto g_matrix_block  = mx.local.block(lbi);
    // index space view on local block obtained from pattern relative to
    // global index space:
    auto g_pattern_block = mx.pattern().local_block(myid, lbi);

    value_t * block_lbegin = g_matrix_block.lbegin();
    value_t * block_lend   = g_matrix_block.lend();
    DASH_LOG_DEBUG("MatrixTest.DelayedAlloc",
                   "local block idx:",   lbi,
                   "block offset:",      g_matrix_block.offsets(),
                   "block extents:",     g_matrix_block.extents(),
                   "block lend-lbegin:", block_lend - block_lbegin);

    // block views should be identical:
    ASSERT_EQ_U(g_matrix_block.extents(),
                g_pattern_block.extents());
    ASSERT_EQ_U(g_matrix_block.offsets(),
                g_pattern_block.offsets());
    // element phase, canonical element offset in block:
    index_t phase = 0;
    for (auto lbv = block_lbegin; lbv != block_lend; ++lbv, ++phase) {
      *lbv = myid + (0.01 * lbi) + (0.0001 * phase);
    }
  }

  mx.barrier();

  if (myid == 0) {
    dash::test::print_matrix("Matrix<3>", mx, 4);
  }

  // Validate values.
  // Testing view specifiers for every index explicitly, intentionally
  // inefficient.
  if (myid == 0) {
    for (index_t i = 0; i < static_cast<index_t>(extent_i); ++i) {
      for (index_t j = 0; j < static_cast<index_t>(extent_j); ++j) {
        for (index_t k = 0; k < static_cast<index_t>(extent_k); ++k) {
          DASH_LOG_TRACE("MatrixTest.DelayedAlloc",
                         "coords:", i, j, k);
          // global coordinate:
          std::array<index_t, 3> gcoords {{ i, j, k }};
          // block index in global memory space:
          auto block_index   = mx.pattern().block_at(gcoords);
          // block index in local memory space:
          auto lbi           = mx.pattern().local_block_at(gcoords).index;
          // block at global block index:
          auto block_extents = mx.pattern().block(block_index).extents();
          auto block_i_space = index_space_t(block_extents);
          auto block_unit    = mx.pattern().unit_at(gcoords);
          // Cartesian offsets of element in block:
          std::array<index_t, 3> phase_coords {{ i % tilesize_i,
                                                 j % tilesize_j,
                                                 k % tilesize_k }};
          DASH_LOG_TRACE("MatrixTest.DelayedAlloc",
                         "block extents:", block_extents,
                         "phase coords:",  phase_coords);
          // canonical offset of element in block:
          index_t phase     = block_i_space.at(phase_coords);
          value_t expected  = block_unit + (0.01 * lbi) + (0.0001 * phase);
          value_t actual    = mx[i][j][k];
          DASH_LOG_TRACE("MatrixTest.DelayedAlloc",
                         "coords:",      i, j, k,
                         "block index:", block_index,
                         "unit:",        block_unit,
                         "phase:",       phase_coords, "=", phase,
                         "expected:",    expected,
                         "actual:",      actual);
          EXPECT_EQ_U(expected, actual);
        }
      }
    }
  }


  // re-allocate to test variadic allocate
  mx.deallocate();

  mx.allocate(
    extent_i,
    extent_j,
    extent_k,
    dash::TILE(tilesize_i),
    dash::TILE(tilesize_j),
    dash::TILE(tilesize_k),
    teamspec
  );
}

TEST_F(MatrixTest, PatternScope)
{
  typedef dash::TilePattern<2>            pattern_t;
  typedef typename pattern_t::index_type  index_t;
  typedef int                             value_t;

  value_t block_size_x = 5;
  value_t block_size_y = 5;
  value_t extent_x     = dash::size() * block_size_x;
  value_t extent_y     = dash::size() * block_size_y;

  auto & team = dash::Team::All();
  dash::TeamSpec<2>         ts(team);
  dash::SizeSpec<2>         ss(extent_y, extent_x);
  dash::DistributionSpec<2> ds(dash::TILE(block_size_y),
                               dash::TILE(block_size_x));

  dash::NArray<value_t, 2, index_t, pattern_t> matrix;

  {
    pattern_t pattern(
         ss,
         ds,
         ts,
         team);
    matrix.allocate(pattern);
  }
  if (dash::myid().id == 0) {
    matrix[0][0] = 123;
  }

  matrix.barrier();

  EXPECT_EQ(static_cast<int>(matrix[0][0]), 123);
}

/**
 * Allocate a matrix with extents that cannot fit into full blocks
 */
TEST_F(MatrixTest, UnderfilledPattern)
{
  typedef dash::Pattern<2, dash::ROW_MAJOR> pattern_t;

  auto team_size    = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto block_size_x = 10;
  auto block_size_y = 15;
  auto ext_x        = (block_size_x * teamspec_2d.num_units(0)) - 3;
  auto ext_y        = (block_size_y * teamspec_2d.num_units(1)) - 1;

  auto size_spec    = dash::SizeSpec<2>(ext_x, ext_y);

  dash::Matrix<
    int, 2,
    typename pattern_t::index_type,
    pattern_t
  > matrix_a(size_spec);

  // test bottom right corner
  if (dash::myid().id == 0) {
    matrix_a[ext_x - 1][ext_y - 1] = 10;
    ASSERT_EQ( matrix_a[ext_x - 1][ext_y - 1], 10 );
  }

  matrix_a.deallocate();

  // Check BlockPattern
  const pattern_t pattern(
    size_spec,
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y)),
    teamspec_2d,
    dash::Team::All());

  dash::Matrix<
    int, 2,
    typename pattern_t::index_type,
    pattern_t
  > matrix_b;

  matrix_b.allocate(pattern);
}

/**
 * Check local extents vs. global extents in 2D matrix with BLOCKED
 * distribution pattern and underfilled blocks.
 */
TEST_F(MatrixTest, UnderfilledBlockedPatternExtents)
{
  typedef dash::default_extent_t   extent_t;
  typedef dash::default_index_t     index_t;

  dart_unit_t myid= dash::myid();
  auto numunits = dash::Team::All().size();

  dash::TeamSpec<2> teamspec( numunits, 1 );
  teamspec.balance_extents();

  extent_t w = 13;
  extent_t h =  7;

  auto distspec = dash::DistributionSpec<2>(dash::BLOCKED, dash::BLOCKED);

  dash::NArray<uint32_t, 2>
    matrix(dash::SizeSpec<2>(h, w),
           distspec,
           dash::Team::All(),
           teamspec);

  auto corner = matrix.pattern().global( {0,0} );

  auto lw = (corner[1] + matrix.local.extent(1) < w )
               ? matrix.local.extent(1)
               : w - corner[1];
  auto lh = (corner[0] + matrix.local.extent(0) < h )
               ? matrix.local.extent(0)
               : h - corner[0];

  EXPECT_LE_U( corner[1] + matrix.local.extent(1), w );
  EXPECT_LE_U( corner[0] + matrix.local.extent(0), h );
}

TEST_F(MatrixTest, UnderfilledLocalViewSpec){
  auto myid     = dash::myid();
  auto numunits = dash::Team::All().size();
  dash::TeamSpec<2> teamspec( numunits, 1 );
  teamspec.balance_extents();

  uint32_t w= 13;
  uint32_t h= 7;
  auto distspec= dash::DistributionSpec<2>( dash::BLOCKED, dash::BLOCKED );
  dash::NArray<uint32_t, 2> narray( dash::SizeSpec<2>( h, w ),
      distspec, dash::Team::All(), teamspec );

  narray.barrier();
  
  if ( 0 == myid ) {
    LOG_MESSAGE("global extent is %lu x %lu",
                narray.extent(0), narray.extent(1));
  }
  LOG_MESSAGE("local extent is %lu x %lu",
              narray.local.extent(0), narray.local.extent(1));

  narray.barrier();

  // test lbegin, lend
  std::fill(narray.lbegin(), narray.lend(), 1);
  std::for_each(narray.lbegin(), narray.lend(), 
      [](uint32_t & el){
        ASSERT_EQ_U(el, 1);
      });
  dash::barrier();
  // test local view
  std::fill(narray.local.begin(), narray.local.end(), 2);
  std::for_each(narray.local.begin(), narray.local.end(), 
      [](uint32_t el){
        ASSERT_EQ_U(el, 2);
      });

  uint32_t elementsvisited = std::distance(narray.lbegin(), narray.lend());  
  auto local_elements= narray.local.extent(0) * narray.local.extent(1);
  
  ASSERT_EQ_U(elementsvisited, local_elements);
  ASSERT_EQ_U(elementsvisited, narray.local.size());

  elementsvisited = std::distance(narray.local.begin(), narray.local.end());
  ASSERT_EQ_U(elementsvisited, local_elements);
}


TEST_F(MatrixTest, SimpleConstructor)
{
  auto ext_x = dash::size();
  auto ext_y = dash::size() * 5;
  dash::Matrix<int, 2> matrix(ext_x, ext_y);

  dash::fill(matrix.begin(), matrix.end(), dash::myid().id);

  matrix.barrier();

  ASSERT_EQ_U(ext_x, matrix.extent(0));
  ASSERT_EQ_U(ext_y, matrix.extent(1));
}

TEST_F(MatrixTest, MatrixLBegin)
{
  auto myid  = dash::myid();
  auto ext_x = dash::size();
  auto ext_y = dash::size() * 5;

  dash::Matrix<int, 2> matrix(ext_x, ext_y);

  dash::fill(matrix.begin(), matrix.end(), myid);
  matrix.barrier();

  int * l_first = matrix.lbegin();

  EXPECT_EQ_U(myid, static_cast<int>(*(matrix.lbegin())));
  EXPECT_EQ_U(myid, static_cast<int>(*(matrix.local.block(0).begin())));
  EXPECT_EQ_U(myid, static_cast<int>(*(matrix.local.begin())));
}

TEST_F(MatrixTest, DelayedPatternAllocation)
{
  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type index_t;

  auto block_size_x = dash::size();
  auto block_size_y = dash::size();
  dash::NArray<int, 2, index_t, pattern_t > matrix;

  {
    auto & team = dash::Team::All();
    dash::TeamSpec<2>         ts(team);
    dash::SizeSpec<2>         ss(block_size_x, block_size_y);
    dash::DistributionSpec<2> ds(dash::TILE(1),dash::TILE(1));

    pattern_t pattern(
          ss,
          ds,
          ts,
          team);
    matrix.allocate(pattern);
  }
  auto id = dash::myid().id;
  matrix(id,id) = id;
  EXPECT_EQ(id, matrix[id][id]);
}

TEST_F(MatrixTest, CopyRow)
{
  typedef int value_t;

  auto team_size = dash::Team::All().size();
  auto myid      = dash::Team::All().myid().id;

  size_t n_lextent = 10;

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto tspec_ny = teamspec_2d.extents()[0];
  auto tspec_nx = teamspec_2d.extents()[1];

  DASH_LOG_DEBUG("MatrixTest.CopyRow", "balanced team spec:",
                 tspec_ny, "x", tspec_nx);

  dash::SizeSpec<2>         sspec(tspec_ny * n_lextent,
                                  tspec_nx * n_lextent);
  dash::DistributionSpec<2> dspec(dash::BLOCKED,
                                  dash::BLOCKED);

  dash::Matrix<value_t, 2>  matrix(sspec, dspec,
                                   dash::Team::All(), teamspec_2d);

  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", matrix.lend() - matrix.lbegin());
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", matrix.local.size());
  for (int l = 0; l < matrix.local.size(); l++) {
    matrix.local.begin()[l] = ((myid + 1) * 1000) + l;
  }
  dash::barrier();

  if (myid == 0) {
    dash::test::print_matrix("Matrix<2>", matrix, 2);
  }
  dash::barrier();

  auto row      = matrix.local.row(0);
  auto row_size = row.size();
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", row_size);
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", row.extent(0));
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", row.extent(1));

  dash::barrier();
  dash::test::print_matrix("Matrix<2>.local.row(0)", row, 2);

  auto l_prange = dash::local_range(row.begin(), row.end());
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow",
                     static_cast<const value_t *>(l_prange.begin));
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow",
                     static_cast<const value_t *>(l_prange.end));
  auto l_irange = dash::local_index_range(row.begin(), row.end());
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", static_cast<int>(l_irange.begin));
  DASH_LOG_DEBUG_VAR("MatrixTest.CopyRow", static_cast<int>(l_irange.end));

  EXPECT_EQ_U(row_size, l_irange.end - l_irange.begin);
  EXPECT_EQ_U(row_size, l_prange.end - l_prange.begin);

  EXPECT_EQ_U(1,         decltype(row)::ndim());
  EXPECT_EQ_U(n_lextent, row_size);

  EXPECT_EQ_U(n_lextent, row.extents()[1]);

  // Check values and test for each expression:
  int li = 0;
  for (auto l_row_val : row) {
    value_t expected = ((myid + 1) * 1000) + li;
    value_t actual   = l_row_val;
    EXPECT_EQ_U(expected, actual);
    li++;
  }

  std::vector<value_t> tmp(row_size);
  auto copy_end = dash::copy(row.begin(), row.end(),
                             tmp.data());

  EXPECT_EQ_U(row_size, copy_end - tmp.data());

  li = 0;
  for (auto l_copy_val : tmp) {
    value_t expected = ((myid + 1) * 1000) + li;
    value_t actual   = l_copy_val;
    EXPECT_EQ_U(expected, actual);
    li++;
  }
}

TEST_F(MatrixTest, ConstMatrix)
{
  typedef dash::BlockPattern<2>            pattern_t;
  typedef typename pattern_t::index_type     index_t;

  int block_rows = 3;
  int block_cols = 4;

  int nrows = dash::size() * block_rows * 2;
  int ncols = dash::size() * block_cols;

  dash::Matrix<int, 2, index_t, pattern_t> matrix(
      dash::SizeSpec<2>(nrows, ncols));

  if (dash::myid().id == 0) {
    DASH_LOG_DEBUG_VAR("MatrixTest.ConstMatrix",
                       matrix.pattern().blockspec());
    DASH_LOG_DEBUG_VAR("MatrixTest.ConstMatrix",
                       matrix.pattern().teamspec());
  }
  
  auto const & matrix_by_ref = matrix;
  auto       & matrix_local  = matrix.local;
  
  dash::fill(matrix.begin(), matrix.end(), 0);
  dash::barrier();
  
  int el = matrix(0,0);
  el = matrix[0][0];
  ASSERT_EQ_U(el, 0);
  
  el = matrix.local[0][0];
  ASSERT_EQ_U(el, 0);

  el = *(matrix.local.lbegin());
  ASSERT_EQ_U(el, 0);

  dash::barrier();
  el = ++(*(matrix.local.lbegin()));
  ASSERT_EQ_U(el, 1);
  el = ++(*(matrix.local.row(0).lbegin()));
  ASSERT_EQ_U(el, 2);
  matrix.barrier();

  // test access using const & matrix
  el = matrix_by_ref[0][0];
  ASSERT_EQ_U(el, 2);

  el = matrix_by_ref.local[0][0];
  ASSERT_EQ_U(el, 2);

  // should not compile
  // el = ++(*(matrix_by_ref.local.lbegin()));
  // el = ++(*(matrix_by_ref.local.row(0).lbegin()));
  // matrix_by_ref.local.row(0)[0] = 5;
  
  // test access using non-const & matrix.local
  matrix.barrier();
  *(matrix_local.lbegin()) = 5;
}

template <
  class MatrixT,
  class ValueT = typename MatrixT::value_type >
ValueT local_sum_rows(const int       nelts,
                      const MatrixT & matIn)
{
  auto   lclRows   = matIn.pattern().local_extents()[0];
  ValueT local_sum = 0;

  // Accumulate local values by row to test combinations of
  // `sub` and `local` view qualifiers:
  ValueT const * mPtr;
  for (int i = 0; i < lclRows; ++i) {
    mPtr = matIn.local.row(i).lbegin();

    for (int j = 0; j < nelts; ++j) {
      local_sum += *(mPtr++);
    }
  }
  return local_sum;
}

template <
  class MatrixT,
  class ValueT = typename MatrixT::value_type >
ValueT global_sum_rows(const int       nelts,
                       const MatrixT & matIn)
{
  auto   glbRows    = matIn.pattern().extents()[0];
  ValueT global_sum = 0;

  for (int i = 0; i < glbRows; ++i) {
    for (const auto & row_val : matIn.row(i)) {
      global_sum += row_val;
    }
  }
  return global_sum;
}

template <
  class MatrixT,
  class ValueT = typename MatrixT::value_type >
ValueT global_sum_elems(const int       nelts,
                        const MatrixT & matIn)
{
  auto   glbRows    = matIn.pattern().extents()[0];
  auto   glbCols    = matIn.pattern().extents()[1];
  ValueT global_sum = 0;

  for (int i = 0; i < glbRows; ++i) {
    for (int j = 0; j < glbCols; ++j) {
      global_sum += matIn(i,j);
    }
  }
  return global_sum;
}

TEST_F(MatrixTest, ConstMatrixRefs)
{
  using value_t = unsigned int;
  using uint    = unsigned int;

  uint myid = static_cast<uint>(dash::Team::GlobalUnitID().id);

  const uint nelts = 40;

  dash::NArray<value_t, 2> mat(nelts, nelts);

  // Initialize matrix values:
  uint counter = myid + 20;
  if (0 == myid) {
    for (uint *i = mat.lbegin(); i < mat.lend(); ++i) {
      *i = ++counter;
    }
  }
  dash::barrier();

  auto local_rows_sum  = local_sum_rows(nelts, mat);
  auto local_range_sum = std::accumulate(
                           mat.lbegin(),
                           mat.lend(),
                           0, std::plus<value_t>());

  EXPECT_EQ_U(local_range_sum, local_rows_sum);

  dash::barrier();

  auto global_rows_sum  = global_sum_rows(nelts, mat);
  auto global_elems_sum = global_sum_elems(nelts, mat);
  auto global_range_sum = std::accumulate(
                            mat.begin(),
                            mat.end(),
                            0, std::plus<value_t>());

  EXPECT_EQ_U(global_range_sum, global_rows_sum);
  EXPECT_EQ_U(global_range_sum, global_elems_sum);
}


TEST_F(MatrixTest, LocalMatrixRefs)
{
  using value_t = unsigned int;
  using uint    = unsigned int;

  uint myid = static_cast<uint>(dash::Team::GlobalUnitID().id);

  const uint nelts = 40;

  dash::NArray<value_t, 2> mat(nelts, nelts);

  for (int i = 0; i < mat.local.extent(0); ++i) {
    auto lref = mat.local[i];
    for (int j = 0; j < mat.local.extent(1); ++j) {
      lref[j] = i*1000 + j;
    }
  }

  // full operator(...)
  for (int i = 0; i < mat.local.extent(0); ++i) {
    for (int j = 0; j < mat.local.extent(1); ++j) {
      ASSERT_EQ_U(mat.local(i, j), i*1000 + j);
    }
  }

  // partial operator(...)
  for (int i = 0; i < mat.local.extent(0); ++i) {
    auto lref = mat.local[i];
    for (int j = 0; j < mat.local.extent(1); ++j) {
      ASSERT_EQ_U(lref(j), i*1000 + j);
    }
  }

  // lbegin, lend
  int cnt = 0;
  for (auto i = mat.local.lbegin(); i != mat.local.lend(); ++i) {
      ASSERT_EQ_U(*i, (cnt / nelts)*1000 + (cnt % nelts));
      ++cnt;
  }
}

TEST_F(MatrixTest, SubViewMatrix3Dim)
{
  int dim_0_ext  = dash::size();
  int dim_1_ext  = 3;
  int dim_2_ext  = 2;

  int sub_0_size = dim_1_ext * dim_2_ext;

  dash::NArray<double, 3> matrix(dim_0_ext, dim_1_ext, dim_2_ext);

  EXPECT_EQ_U(3, matrix.ndim());
  EXPECT_EQ_U(2, matrix[0].ndim());
  EXPECT_EQ_U(1, matrix[0][0].ndim());

  DASH_LOG_DEBUG_VAR("MatrixTest.SubViewMatrix3Dim", matrix.extents());

  EXPECT_EQ_U(dim_0_ext, matrix.extent(0));
  EXPECT_EQ_U(dim_1_ext, matrix.extent(1));
  EXPECT_EQ_U(dim_2_ext, matrix.extent(2));

  dash::fill(matrix.begin(), matrix.end(), 0.0);
  
  if (dash::myid() == 0) {
    for (int i = 0; i < matrix.extent(0); ++i) {
      for (int j = 0; j < matrix.extent(1); ++j) {
        for (int k = 0; k < matrix.extent(2); ++k) {
          matrix[i][j][k] = 0.1   * i +
                            0.01  * j +
                            0.001 * k;
        } } }
  }
  matrix.barrier();

  for (double * lp = matrix.lbegin(); lp != matrix.lend(); ++lp) {
    *lp += dash::myid().id;
  }
  matrix.barrier();

  EXPECT_EQ_U(1,         matrix[0].extent(0));
  EXPECT_EQ_U(dim_1_ext, matrix[0].extent(1));
  EXPECT_EQ_U(dim_2_ext, matrix[0].extent(2));

  if (dash::myid() == 0) {
    dash::test::print_matrix("Matrix<3>", matrix, 3);
    for (int i = 0; i < matrix.extent(0); ++i) {
      DASH_LOG_DEBUG_VAR("MatrixTest.SubViewMatrix3Dim",
                         matrix[i].viewspec());
      for (int j = 0; j < matrix.extent(1); ++j) {
        DASH_LOG_DEBUG_VAR("MatrixTest.SubViewMatrix3Dim",
                           matrix[i][j].viewspec());
        std::vector<double> row(matrix[i][j].begin(),
                                matrix[i][j].end());
        DASH_LOG_DEBUG("MatrixTest.SubViewMatrix3Dim",
                       "matrix[",i,"][",j,"]", row);
      }
    }
  }
  matrix.barrier();

  EXPECT_EQ_U(sub_0_size, matrix[0].size());
  EXPECT_EQ_U(sub_0_size, std::distance(matrix[0].begin(),
                                        matrix[0].end()));
  
  if (dash::myid().id == 0) {
  int visited = 0;
    for (auto it = matrix[0].begin(); it != matrix[0].end();
         ++it, ++visited) {
      double val = *it;
    }
    EXPECT_EQ_U(visited, sub_0_size);
  }
}

TEST_F(MatrixTest, MoveSemantics){
  using matrix_t = dash::NArray<double, 2>;
  // move construction
  {
    matrix_t matrix_a(10, 5);

    *(matrix_a.lbegin()) = 5;
    dash::barrier();

    matrix_t matrix_b(std::move(matrix_a));
    int value = *(matrix_b.lbegin());
    ASSERT_EQ_U(value, 5);
  }
  dash::barrier();
  //move assignment
  {
    matrix_t matrix_a(10, 5);
    {
      matrix_t matrix_b(8, 5);

      *(matrix_a.lbegin()) = 1;
      *(matrix_b.lbegin()) = 2;
      matrix_a = std::move(matrix_b);
      // leave scope of matrix_b
    }
    ASSERT_EQ_U(*(matrix_a.lbegin()), 2);
  }
  dash::barrier();
  // swap
  {
    matrix_t matrix_a(10, 5);
    matrix_t matrix_b(8, 5);

    *(matrix_a.lbegin()) = 1;
    *(matrix_b.lbegin()) = 2;

    std::swap(matrix_a, matrix_b);
    ASSERT_EQ_U(*(matrix_a.lbegin()), 2);
    ASSERT_EQ_U(*(matrix_b.lbegin()), 1);
  }
}

// test for issue 532
TEST_F(MatrixTest, LocalDiagonal){
  dash::TeamSpec<2> ts(dash::size(), 1);
  ts.balance_extents();

  auto ext_per_unit = 5;
  auto tilesize     = 4;
  auto tile         = dash::TILE(tilesize);
  auto global_ext   = ext_per_unit*tilesize;

  dash::NArray<int, 2, dash::default_index_t, dash::TilePattern<2>> mat(global_ext, global_ext, tile, tile, ts);

  // fill with neutral value -1
  dash::fill(mat.begin(), mat.end(), -1);
  mat.barrier();

  // set the diagonal
  for (size_t i = 0; i < mat.extent(0); ++i) {
    if (mat(i, i).is_local()){
      DASH_LOG_DEBUG("Element is local (i,i)", i, i);
      mat(i, i) = dash::myid();
    }
  }
  mat.barrier();

  // check that local range only contains zero or unitid
  auto local_size   = mat.local_size();
  if(local_size > 0){
    LOG_MESSAGE("Validate local memory");
    for(int i=0; i<local_size; ++i){
      int  value    = *(mat.lbegin()+i);
      bool valid    = (value == -1) || (value == dash::myid().id);
      ASSERT_EQ_U(valid, true);
    }
  } else {
    LOG_MESSAGE("No local elements");
  }
  // check global diagonal
  LOG_MESSAGE("Validate global diagonal");
  const auto & pattern = mat.pattern();
  for(int i=0; i<mat.extent(0); ++i){
    int value_fort = mat(i,i);  // fortran style
    int value_sub  = mat[i][i]; // c-array style
    ASSERT_EQ_U(value_fort, value_sub);
    // check if diag value equals owner-unit-id
    int unit = pattern.local({i,i}).unit;
    DASH_LOG_DEBUG("Owning Unit (i,i,unit)", i, i, unit);
    ASSERT_EQ_U(value_sub, unit);
  }
}

