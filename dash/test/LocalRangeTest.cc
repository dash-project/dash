#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "LocalRangeTest.h"

TEST_F(LocalRangeTest, ArrayBlockcyclic)
{
  const size_t blocksize        = 2;
  const size_t num_blocks_local = 3;
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
  auto l_idx_range_half = dash::local_index_range(
                            array.begin() + ((array.size() / 2) + 1),
                            array.end());
  LOG_MESSAGE("Local index range: lbegin:%d lend:%d",
              l_idx_range_half.begin,
              l_idx_range_half.end);
  ASSERT_EQ_U(3, l_idx_range_half.begin);
  ASSERT_EQ_U(6, l_idx_range_half.end);
}

