#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "LocalRangeTest.h"

TEST_F(LocalRangeTest, ArrayBlockcyclic)
{
  const size_t blocksize        = 3;
  const size_t num_blocks_local = 2;
  const size_t num_elem_local   = num_blocks_local * blocksize;
  size_t num_elem_total         = _dash_size * num_elem_local;
  // Identical distribution in all ranges:
  dash::Array<int> array(num_elem_total,
                         dash::BLOCKCYCLIC(blocksize));
  // Should return full local index range from 0 to 6:
  auto l_idx_range_full = dash::local_index_range(
                            array.begin(),
                            array.end());

  ASSERT_EQ_U(l_idx_range_full.begin, 0);
  ASSERT_EQ_U(l_idx_range_full.end, 6);
  // Local index range from second half of global range, so every unit should
  // start its local range from the second block:
  LOG_MESSAGE("array.size: %d", array.size());
  auto l_idx_range_half = dash::local_index_range(
                            array.begin() + (array.size() / 2),
                            array.end());
  LOG_MESSAGE("Local index range: lbegin:%d lend:%d",
              l_idx_range_half.begin,
              l_idx_range_half.end);
  ASSERT_EQ_U(3, l_idx_range_half.begin);
  ASSERT_EQ_U(6, l_idx_range_half.end);
}

TEST_F(LocalRangeTest, ArrayBlockedWithOffset)
{
  if (_dash_size < 2) {
    return;
  }

  const size_t block_size      = 20;
  const size_t num_elems_total = _dash_size * block_size;
  // Start at global index 5:
  const size_t offset          = 5;
  // Followed by 2.5 blocks:
  const size_t num_elems       = (block_size * 2) + (block_size / 2);

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  LOG_MESSAGE("global index range: begin:%d end:%d",
              offset, offset + num_elems);
  auto l_idx_range = dash::local_index_range(
                       array.begin() + offset,
                       array.begin() + offset + num_elems);
  LOG_MESSAGE("local index range: begin:%d - end:%d",
              l_idx_range.begin, l_idx_range.end);
  if (dash::myid() == 0) {
    // Local range of unit 0 should start at offset:
    ASSERT_EQ_U(offset, l_idx_range.begin);
  } else if (dash::myid() == 1) {
    // Local range of unit 1 should span full local range:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(block_size, l_idx_range.end);
  } else if (dash::myid() == 2) {
    // Local range of unit 3 should span 5 units (offset) plus half a block:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(offset + (block_size / 2), l_idx_range.end);
  } else {
    // All other units should have an empty local range:
    ASSERT_EQ_U(0, l_idx_range.begin);
    ASSERT_EQ_U(0, l_idx_range.end);
  }
}
