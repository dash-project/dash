#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "MaxElementTest.h"

TEST_F(MaxElementTest, TestFindArrayDefault)
{
  // Initialize global array:
  Array_t array(_num_elem);
  Element_t max_value = (array.size() * 11) + 1;
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = i * 11;
      array[i] = value;
    }
    // Set maximum element in the center position:
    index_t max_pos = array.size() / 2;
    array[max_pos] = max_value;
  }
  // Wait for array initialization
  array.barrier();
  // Run max_element on complete array
  auto found_git = dash::max_element(
                     array.begin(),
                     array.end());
  // Check that a maximum has been found (found != last):
  EXPECT_NE_U(found_git, array.end());
  // Check maximum value found
  Element_t found_max = *found_git;
  LOG_MESSAGE("Expected max value: %d, found max value %d",
              max_value, found_max);
  EXPECT_EQ(max_value, found_max);
}

TEST_F(MaxElementTest, TestFindArrayDistributeBlockcyclic)
{
  // Using a prime as block size for 'inconvenient' strides.
  int block_size   = 7;
  size_t num_units = dash::Team::All().size();
  LOG_MESSAGE("Units: %d, block size: %d, elements: %d",
              num_units, block_size, _num_elem);
  // Initialize global array:
  Array_t array(_num_elem, dash::BLOCKCYCLIC(block_size));
  Element_t max_value = (array.size() * 23) + 1;
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = i * 23;
      array[i] = value;
    }
    // Set maximum element somewhere in the first half:
    index_t max_pos = array.size() / 3;
    array[max_pos] = max_value;
  }
  // Wait for array initialization
  array.barrier();
  auto found_git = dash::max_element(
                     array.begin(),
                     array.end());
  // Check that a maximum has been found (found != last):
  EXPECT_NE_U(found_git, array.end());
  // Check maximum value found
  Element_t found_max = *found_git;
  LOG_MESSAGE("Expected max value: %d, found maximum value %d",
              max_value, found_max);
  EXPECT_EQ(max_value, found_max);
}

TEST_F(MaxElementTest, TestFindArrayUnderfilled)
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
  // Initialize global array:
  Array_t array(num_elem, dash::BLOCKCYCLIC(block_size));
  Element_t max_value = (array.size() * 23) + 1;
  if (dash::myid() == 0) {
    for (auto i = 0; i < array.size(); ++i) {
      Element_t value = i * 23;
      array[i] = value;
    }
    // Set maximum element in the last position which is located
    // in the underfilled block, for extra nastyness:
    index_t max_pos = array.size() - 1;
    array[max_pos] = max_value;
  }
  // Wait for array initialization
  array.barrier();
  auto found_git = dash::max_element(
                     array.begin(),
                     array.end());
  // Check that a maximum has been found (found != last):
  EXPECT_NE_U(found_git, array.end());
  // Check maximum value found
  Element_t found_max = *found_git;
  LOG_MESSAGE("Expected max value: %d, found maximum value %d",
              max_value, found_max);
  EXPECT_EQ(max_value, found_max);
}
