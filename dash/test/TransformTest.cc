#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "TransformTest.h"

#include <array>

TEST_F(TransformTest, ArrayLocalPlusLocal)
{
  // Local input and output ranges, does not require communication
  const size_t num_elem_local = 5;
  size_t num_elem_total       = _dash_size * num_elem_local;
  // Identical distribution in all ranges:
  dash::Array<int> array_in(num_elem_total, dash::BLOCKED);
  dash::Array<int> array_dest(num_elem_total, dash::BLOCKED);

  // Fill ranges with initial values:
  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    array_in.local[l_idx]   = l_idx;
    array_dest.local[l_idx] = 23;
  }

  dash::barrier();

  // Identical start offsets in all ranges (begin() = 0):
  dash::transform<int>(
      array_in.begin(), array_in.end(), // A
      array_dest.begin(),               // B
      array_dest.begin(),               // C = op(A,B)
      dash::plus<int>());               // op

  dash::barrier();

  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    EXPECT_EQ_U(array_in.local[l_idx], l_idx);
    EXPECT_EQ_U(array_dest.local[l_idx], l_idx + 23);
  }

}

TEST_F(TransformTest, ArrayGlobalPlusLocalBlocking)
{
  if (dash::size() == 3) {
    // TODO: Fix this
    SKIP_TEST();
  }

  // Add local range to every block in global range
  const size_t num_elem_local = 5;
  size_t num_elem_total = _dash_size * num_elem_local;
  dash::Array<int> array_dest(num_elem_total, dash::BLOCKED);
  std::array<int, num_elem_local> local;

  EXPECT_EQ_U(num_elem_total, array_dest.size());
  EXPECT_EQ_U(num_elem_local, array_dest.lend() - array_dest.lbegin());

  // Initialize result array: [ 100, 100, ... | 200, 200, ... ]
  int loffs = 0;
  for (auto l_it = array_dest.lbegin(); l_it != array_dest.lend(); ++l_it) {
    *l_it = 10000 + loffs;
    loffs++;
  }

  // Every unit adds a local range of elements to every block in a global
  // array.

  // Initialize local values, e.g. unit 2: [ 2000, 2001, 2002, ... ]
  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    local[l_idx] = (dash::myid() + 1);
  }

  // Accumulate local range to every block in the array:
  for (size_t block_idx = 0; block_idx < _dash_size; ++block_idx) {
    auto block_offset  = block_idx * num_elem_local;
    auto transform_end =
      dash::transform<int>(&(*local.begin()), &(*local.end()), // A
                           array_dest.begin() + block_offset,  // B
                           array_dest.begin() + block_offset,  // B = op(B,A)
                           dash::plus<int>());                 // op
  }

  dash::barrier();

  if (dash::myid() == 0) {
    for (size_t g = 0; g < array_dest.size(); ++g) {
      int val = array_dest[g];
      LOG_MESSAGE("TransformTest.ArrayGlobalPlusLocalBlocking: "
                  "array_dest[%d] = %d", g, val);
    }
  }

  dash::barrier();

  // Verify values in local partition of array:

  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    int expected = (10000 + l_idx) +
                   ((dash::size() * (dash::size() + 1)) / 2);
    LOG_MESSAGE("TransformTest.ArrayGlobalPlusLocalBlocking",
                "array_dest.local[%d]: %d",
                l_idx, &array_dest.local[l_idx]);
    EXPECT_EQ_U(expected, array_dest.local[l_idx]);
  }

  dash::barrier();
}

TEST_F(TransformTest, ArrayGlobalPlusGlobalBlocking)
{
  // Add values in global range to values in other global range
  const size_t num_elem_local = 100;
  size_t num_elem_total = _dash_size * num_elem_local;
  dash::Array<int> array_dest(num_elem_total, dash::BLOCKED);
  dash::Array<int> array_values(num_elem_total, dash::BLOCKED);

  // Initialize result array: [ 100, 100, ... | 200, 200, ... ]
  for (auto l_it = array_dest.lbegin(); l_it != array_dest.lend(); ++l_it) {
    *l_it = (dash::myid() + 1) * 100;
  }

  // Every unit adds a local range of elements to every block in a global
  // array.

  // Initialize values to add, e.g. unit 2: [ 2000, 2001, 2002, ... ]
  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    array_values.local[l_idx] = ((dash::myid() + 1) * 1000) + (l_idx + 1);
  }

  // Accumulate local range to every block in the array:
  dash::transform<int>(array_values.begin(), array_values.end(), // A
                       array_dest.begin(),                       // B
                       array_dest.begin(),                       // B = B op A
                       dash::plus<int>());                       // op

  dash::barrier();

  // Verify values in local partition of array:

  for (size_t l_idx = 0; l_idx < num_elem_local; ++l_idx) {
    int expected = (dash::myid() + 1) * 100 +               // initial value
                   (dash::myid() + 1) * 1000 + (l_idx + 1); // added value
    EXPECT_EQ_U(expected, array_dest.local[l_idx]);
  }
}

TEST_F(TransformTest, MatrixGlobalPlusGlobalBlocking)
{
  // Block-wise addition (a += b) of two matrices
  typedef typename dash::Matrix<int, 2>::index_type index_t;
  dash::global_unit_t myid = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 7;
  size_t tilesize_y  = 3;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  dash::Matrix<int, 2> matrix_a(
                         dash::SizeSpec<2>(
                           extent_cols,
                           extent_rows),
                         dash::DistributionSpec<2>(
                           dash::TILE(tilesize_x),
                           dash::TILE(tilesize_y)));
  dash::Matrix<int, 2> matrix_b(
                         dash::SizeSpec<2>(
                           extent_cols,
                           extent_rows),
                         dash::DistributionSpec<2>(
                           dash::TILE(tilesize_x),
                           dash::TILE(tilesize_y)));
  size_t matrix_size = extent_cols * extent_rows;
  EXPECT_EQ(matrix_size, matrix_a.size());
  EXPECT_EQ(extent_cols, matrix_a.extent(0));
  EXPECT_EQ(extent_rows, matrix_a.extent(1));
  LOG_MESSAGE("Matrix size: %d", matrix_size);

  // Fill matrix
  if(myid == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(size_t i = 0; i < matrix_a.extent(0); ++i) {
      for(size_t k = 0; k < matrix_a.extent(1); ++k) {
        auto value = (i * 1000) + (k * 1);
        matrix_a[i][k] = value * 100000;
        matrix_b[i][k] = value;
      }
    }
  }
  LOG_MESSAGE("Wait for team barrier ...");
  dash::Team::All().barrier();
  LOG_MESSAGE("Team barrier passed");

  LOG_MESSAGE("Test first global block");
  // Offset and extents of first block in global cartesian space:
  auto first_g_block_a = matrix_a.pattern().block(0);
  // Global coordinates of first element in first global block:
  std::array<index_t, 2> first_g_block_a_begin   = { 0, 0 };
//std::array<index_t, 2> first_g_block_a_offsets = first_g_block_a.offsets();
  EXPECT_EQ_U(first_g_block_a_begin,
              first_g_block_a.offsets());

  LOG_MESSAGE("Test first local block");
  // Offset and extents of first block in local cartesian space:
  auto first_l_block_a = matrix_a.pattern().local_block(0);
  // Global coordinates of first element in first local block:
  std::array<index_t, 2> first_l_block_a_begin   = {
                           static_cast<index_t>(myid * tilesize_x), 0 };
  std::array<index_t, 2> first_l_block_a_offsets = first_l_block_a.offsets();
  EXPECT_EQ_U(first_l_block_a_begin,
              first_l_block_a_offsets);
}
