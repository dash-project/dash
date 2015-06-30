#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "MinElementTest.h"

TEST_F(MinElementTest, TestFindArrayDefault)
{
  Element_t min_value = 11;
  // Initialize global array:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 41;
      LOG_MESSAGE("Setting array[%d] = %d", i, value);
      array[i] = value;
    }
    // Set minimum element in the center position:
    index_t min_pos = array.size() / 2;
    LOG_MESSAGE("Setting array[%d] = %d (min)", 
                min_pos, min_value);
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  // Run min_element on complete array
  dash::GlobPtr<Element_t> found_gptr =
    dash::min_element(
      array.begin(),
      array.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, nullptr);
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

TEST_F(MinElementTest, TestFindArrayDistributeBlockcyclic)
{
  // Using a prime as block size for 'inconvenient' strides.
  int block_size   = 7;
  size_t num_units = dash::Team::All().size();
  LOG_MESSAGE("Units: %d, block size: %d, elements: %d",
              num_units, block_size, _num_elem);
  Element_t min_value = 19;
  // Initialize global array:
  Array_t array(_num_elem, dash::BLOCKCYCLIC(block_size));
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 23;
      LOG_MESSAGE("Setting array[%d] = %d", i, value);
      array[i] = value;
    }
    // Set minimum element somewhere in the first half:
    index_t min_pos = array.size() / 3;
    LOG_MESSAGE("Setting array[%d] = %d (min)", 
                min_pos, min_value);
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  dash::GlobPtr<Element_t> found_gptr =
    dash::min_element(
      array.begin(),
      array.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, nullptr);
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

TEST_F(MinElementTest, TestFindArrayUnderfilled)
{
  // Choose block size and number of blocks so at least
  // one unit has an empty local range and one unit has an
  // underfilled block.
  // Using a prime as block size for 'inconvenient' strides.
  int block_size   = 19;
  size_t num_units = dash::Team::All().size();
  size_t num_elem  = ((num_units - 1) * block_size) - block_size / 2;
  if (num_units < 2) {
    num_elem = block_size - 1;
  }
  LOG_MESSAGE("Units: %d, block size: %d, elements: %d",
              num_units, block_size, num_elem);
  Element_t min_value = 21;
  // Initialize global array:
  Array_t array(num_elem, dash::BLOCKCYCLIC(block_size));
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 23;
      LOG_MESSAGE("Setting array[%d] = %d", i, value);
      array[i] = value;
    }
    // Set minimum element in the last position which is located
    // in the underfilled block, for extra nastyness:
    index_t min_pos = array.size() - 1;
    LOG_MESSAGE("Setting array[%d] = %d (min)", 
                min_pos, min_value);
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  dash::GlobPtr<Element_t> found_gptr =
    dash::min_element(
      array.begin(),
      array.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, nullptr);
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

TEST_F(MinElementTest, TestFindMatrixDefault)
{
  Element_t min_value = 11;
  size_t extent_cols  = 431;
  size_t extent_rows  = 547;
  int min_pos_x       = 234;
  int min_pos_y       = 534;
  dash::Matrix<Element_t, 2> matrix(
                         dash::SizeSpec<2>(
                           extent_cols, extent_rows),
                         dash::DistributionSpec<2>(
                           dash::NONE, dash::BLOCKCYCLIC(51)));
  size_t matrix_size = extent_cols * extent_rows;
  ASSERT_EQ(matrix_size, matrix.size());
  ASSERT_EQ(extent_cols, matrix.extent(0));
  ASSERT_EQ(extent_rows, matrix.extent(1));
  LOG_MESSAGE("Matrix size: %d", matrix_size);
  // Fill matrix
  if (dash::myid() == 0) {
    LOG_MESSAGE("Assigning matrix values");
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        matrix[i][k] = 20 + (i * 11) + (k * 97);
      }
    }
    LOG_MESSAGE("Setting matrix[%d][%d] = %d (min)", 
                min_pos_x, min_pos_y, min_value);
    matrix[min_pos_x][min_pos_y] = min_value;
  }
  // Units waiting for value initialization
  dash::Team::All().barrier();
  // Run min_element on complete matrix
  dash::GlobPtr<Element_t> found_gptr =
    dash::min_element(
      matrix.begin(),
      matrix.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, nullptr);
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

