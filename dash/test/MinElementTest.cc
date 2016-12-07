#include <libdash.h>
#include <gtest/gtest.h>

#include <limits>

#include "TestBase.h"
#include "MinElementTest.h"

TEST_F(MinElementTest, TestFindArrayDefault)
{
  _num_elem           = dash::Team::All().size();
  Element_t min_value = 11;
  // Initialize global array:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 41;
      array[i] = value;
    }
    // Set minimum element in the center position:
    index_t min_pos = array.size() / 2;
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  LOG_MESSAGE("Waiting for other units to initialize array values");
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");
  // Run min_element on complete array
  auto found_gptr = dash::min_element(array.begin(),
                                      array.end());
  // Check that a minimum has been found (found != last):
  LOG_MESSAGE("Completed dash::min_element");
  // Run min_element on complete array
  EXPECT_NE_U(found_gptr, array.end());
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

TEST_F(MinElementTest, TestArrayDelayedAlloc)
{
  typedef long value_t;

  dash::Array<value_t> array;
  // Delayed allocation:
  array.allocate(10 * dash::Team::All().size(), dash::BLOCKED);
  // Initialize values:
  value_t l_min_exp = std::numeric_limits<value_t>::max();
  auto l_size = array.local.size();
  for (auto li = 0; li < l_size; ++li) {
    value_t value = ((dash::myid() + 1) * 17) + ((l_size - li) * 3);
    if (value % 2 == 0) {
      value = -value;
    }
    array.local[li] = value;
    if (value < l_min_exp) {
      l_min_exp = value;
    }
    LOG_MESSAGE("array.local[%d] = %d", li, value);
  }
  // Wait for all units to initialize values:
  array.barrier();
  // Find local minimum:
  auto lptr_min = dash::min_element(array.local.begin(),
                                    array.local.end());
  LOG_MESSAGE("l_min:%d expected:%d", *lptr_min, l_min_exp);
  ASSERT_EQ_U(l_min_exp, *lptr_min);
  // Find global minimum:
  auto gptr_min = dash::min_element(array.begin(),
                                    array.end());
  value_t g_min = *gptr_min;
  ASSERT_LE_U(g_min, *lptr_min);
  LOG_MESSAGE("g_min: %d", g_min);
}

TEST_F(MinElementTest, TestFindArrayDistributeBlockcyclic)
{
  // Using a prime as block size for 'inconvenient' strides.
  int  block_size = 7;
  auto num_units  = dash::Team::All().size();
  LOG_MESSAGE("Units: %d, block size: %d, elements: %d",
              num_units, block_size, _num_elem);
  Element_t min_value = 19;
  // Initialize global array:
  Array_t array(_num_elem, dash::BLOCKCYCLIC(block_size));
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = (i + 1) * 23;
      array[i] = value;
    }
    // Set minimum element somewhere in the first half:
    index_t min_pos = array.size() / 3;
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  auto found_gptr = dash::min_element(array.begin(),
                                      array.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, array.end());
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
      array[i] = value;
    }
    // Set minimum element in the last position which is located
    // in the underfilled block, for extra nastyness:
    index_t min_pos = array.size() - 1;
    array[min_pos] = min_value;
  }
  // Wait for array initialization
  array.barrier();
  auto found_gptr = dash::min_element(array.begin(),
                                      array.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, array.end());
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

TEST_F(MinElementTest, TestShrinkRange)
{
  dash::Array<int> arr(100);

  // Shrink from front:
  if (dash::myid() == 0) {
    for (auto i = 0; i < arr.size(); ++i) {
      arr[i] = 100 + i;
    }
  }
  arr.barrier();
  auto min_expected = 100;
  for (auto it = arr.begin(); it != arr.end(); ++it) {
    dash::util::Config::set("DASH_ENABLE_LOGGING", true);

    DASH_LOG_TRACE("MinElementTest.TestShrinkRange",
                   "begin at", it.pos());
    auto it_min = dash::min_element(it, arr.end());
    // Test if a minimum element has been found:
    int  min    = *it_min;
    DASH_LOG_TRACE("MinElementTest.TestShrinkRange",
                   "begin at", it.pos(), "minimum:", min);
    EXPECT_NE_U(it_min, arr.end());
    EXPECT_EQ_U(min_expected, min);
    ++min_expected;

    dash::util::Config::set("DASH_ENABLE_LOGGING", false);
  }

  arr.barrier();

  // Shrink from back:
  if (dash::myid() == 0) {
    for (auto i = 0; i < arr.size(); ++i) {
      arr[i] = 100 + arr.size() - 1 - i;
    }
  }
  arr.barrier();
  min_expected = 100;
  for (auto it = arr.end(); it != arr.begin(); --it) {
    dash::util::Config::set("DASH_ENABLE_LOGGING", true);

    DASH_LOG_TRACE("MinElementTest.TestShrinkRange",
                   "end at %d", it.pos());
    auto it_min = dash::min_element(arr.begin(), it);
    // Test if a minimum element has been found:
    int  min    = *it_min;
    DASH_LOG_TRACE("MinElementTest.TestShrinkRange",
                   "end at", it.pos(), "minimum:", min);
    EXPECT_NE_U(it_min, arr.end());
    EXPECT_EQ_U(min_expected, min);
    ++min_expected;

    dash::util::Config::set("DASH_ENABLE_LOGGING", false);
  }
  arr.barrier();
}

TEST_F(MinElementTest, TestFindMatrixDefault)
{
  Element_t min_value = 11;
  size_t num_units    = dash::Team::All().size();
  size_t tilesize_x   = 13;
  size_t tilesize_y   = 17;
  size_t extent_cols  = tilesize_x * 5 * num_units;
  size_t extent_rows  = tilesize_y * 3 * num_units;
  int min_pos_x       = extent_cols / 2;
  int min_pos_y       = extent_rows / 2;
  dash::Matrix<Element_t, 2> matrix(
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
  auto found_gptr = dash::min_element(matrix.begin(),
                                      matrix.end());
  // Check that a minimum has been found (found != last):
  EXPECT_NE_U(found_gptr, matrix.end());
  // Check minimum value found
  Element_t found_min = *found_gptr;
  LOG_MESSAGE("Expected min value: %d, found minimum value %d",
              min_value, found_min);
  EXPECT_EQ(min_value, found_min);
}

