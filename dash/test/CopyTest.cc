
#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "CopyTest.h"

TEST_F(CopyTest, BlockingGlobalToLocalBlock)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int local_copy[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  dash::copy(array.begin(),
             array.begin() + num_elem_per_unit,
             local_copy);
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    ASSERT_EQ_U(static_cast<int>(array[l]),
                local_copy[l]);
  }
}

TEST_F(CopyTest, Blocking2DimGlobalToLocalBlock)
{
  // Copy all elements contained in a single, continuous, 2-dimensional block.
  const size_t block_size_x  = 3;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * _dash_size * 2;
  size_t extent_y            = block_size_y * _dash_size * 2;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / _dash_size;
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;

  LOG_MESSAGE("nunits:%d elem_total:%d elem_per_unit:%d blocks_per_unit:d%",
              _dash_size, num_elem_total,
              num_elem_per_unit, num_blocks_per_unit);

  typedef dash::TilePattern<2> pattern_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_x,
      extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y))
  );

  dash::Matrix<int, 2> matrix(pattern);

  // Assign initial values:
  for (auto lb = 0; lb < num_blocks_per_unit; ++lb) {
    auto lblock         = matrix.local.block(lb);
    auto lblock_extents = lblock.extents();
    LOG_MESSAGE("initialize values in local block %d (size: %d x %d)",
                lb, lblock_extents[0], lblock_extents[1]);
    for (auto by = 0; by < lblock_extents[1]; ++by) {
      for (auto bx = 0; bx < lblock_extents[0]; ++bx) {
        lblock[by][bx] = ((dash::myid() + 1) * 10000) + (by * 100) + bx;
      }
    }
  }

  matrix.barrier();
  
  // Local range to store copy:
  int local_copy[num_elem_per_unit];

}

TEST_F(CopyTest, BlockingGlobalToLocalMasterOnlyAllRemote)
{
  typedef int64_t index_t;
  typedef dash::Array<
            int,
            index_t,
            dash::CSRPattern<1, dash::ROW_MAJOR, index_t>
          > Array_t;
  if (_dash_size < 2) {
    return;
  }
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 250;
  size_t    num_elem_total    = _dash_size * num_elem_per_unit;
  size_t    num_copy_elem     = (_dash_size - 1) * num_elem_per_unit;

  Array_t   array(num_elem_total, dash::BLOCKED);
  auto      l_start_idx       = array.pattern().lbegin();
  auto      l_end_idx         = array.pattern().lend();

  LOG_MESSAGE("lstart:%d lend:%d ncopy:%d",
              l_start_idx, l_end_idx, num_copy_elem);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int   local_copy[num_copy_elem];
  int * dest_first = local_copy;
  int * dest_last  = local_copy;
  if (dash::myid() == 0) {
    // Copy elements in front of local range:
    LOG_MESSAGE("Copying from global range (%d-%d]",
                0, l_start_idx);
    dest_first = dash::copy(
                   array.begin(),
                   array.begin() + l_start_idx,
                   dest_first);
    // Copy elements after local range:
    LOG_MESSAGE("Copying from global range (%d-%d]",
                l_end_idx, array.size());
    dest_last  = dash::copy(
                   array.begin() + l_end_idx,
                   array.end(),
                   dest_first);
    LOG_MESSAGE("Validating elements");
    int l = 0;
    for (auto g = 0; g < array.size(); ++g) {
      if (array.pattern().unit_at(g) != dash::myid()) {
        int expected = array[g];
        LOG_MESSAGE("Validating value at global index %d (local: %d) = %d",
                    g, l, expected);
        ASSERT_EQ_U(expected, local_copy[l]);
        ++l;
      }
    }
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalBarrierUnaligned)
{
  dart_unit_t myid       = dash::myid();
  size_t num_units       = dash::Team::All().size();
  size_t num_elems_unit  = 20;
  size_t start_index     = 7;
  size_t num_elems_copy  = 20;
  size_t num_elems_total = num_elems_unit * num_units;

  int    local_array[num_elems_copy];
  dash::Array<int> array(num_elems_total);

  LOG_MESSAGE("Elements per unit: %d", num_elems_unit);
  LOG_MESSAGE("Start index:       %d", start_index);
  LOG_MESSAGE("Elements to copy:  %d", num_elems_copy);
  LOG_MESSAGE("Array size:        %d", array.size());

  std::fill(array.lbegin(), array.lend(), myid);

  array.barrier();

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);

  for(int i = 0; i < num_elems_copy; ++i) {
    LOG_MESSAGE("Testing local element %d = %d", i, local_array[i]);
  }

  array.barrier();

  for (auto l = 0; l < num_elems_unit; ++l) {
    ASSERT_EQ_U(local_array[l],
                static_cast<int>(array[start_index + l]));
  }
}

TEST_F(CopyTest, BlockingLocalToGlobalBlock)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  // Global target range:
  dash::Array<int> array(num_elem_total, dash::BLOCKED);
  // Local range to copy:
  int local_range[num_elem_per_unit];

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = 0;
    local_range[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Copy values from local range to remote global range.
  // All units (u) copy into block (nblocks-1-u), so unit 0 copies into last
  // block.
  auto block_offset  = _dash_size - 1 - dash::myid();
  auto global_offset = block_offset * num_elem_per_unit;
  dash::copy(local_range,
             local_range + num_elem_per_unit,
             array.begin() + global_offset);

  array.barrier();

  for (auto l = 0; l < num_elem_per_unit; ++l) {
    ASSERT_EQ_U(local_range[l],
                static_cast<int>(array[global_offset + l]));
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalSubBlock)
{
  // Copy all elements contained in a single, continuous block, 
  // starting from an index unequal 0.
  const size_t num_elems_per_unit = 20;
  const size_t num_elems_total    = _dash_size * num_elems_per_unit;
  // Number of elements to copy
  const size_t num_elems_copy     = 5;
  // Index to start the copy
  const size_t start_index        = 5;

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int local_array[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests local-to-local
  // copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);

  array.barrier();

  for (int l = 0; l < num_elems_copy; ++l) {
    ASSERT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalSubBlockTwoUnits)
{
  // Copy all elements contained in a single, continuous block, 
  // starting from an index unequal 0.
  
  if(_dash_size < 2)
    return;

  const size_t num_elems_per_unit = 20;
  const size_t num_elems_total    = _dash_size * num_elems_per_unit;
  // Number of elements to copy
  const size_t num_elems_copy     = num_elems_per_unit;
  // Index to start the copy
  const size_t start_index        = 5;

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int local_array[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests local-to-local
  // copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);
  for (int l = 0; l < num_elems_copy; ++l) {
    LOG_MESSAGE("Testing local element %d = %d", l, local_array[l]);
  }
  for (int l = 0; l < num_elems_copy; ++l) {
    ASSERT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalSubBlockThreeUnits)
{
  // Copy all elements contained in a single, continuous block, 
  // starting from an index unequal 0.
  
  if(_dash_size < 3)
    return;
  
  const size_t num_elems_per_unit = 20;
  const size_t num_elems_total    = _dash_size * num_elems_per_unit;
  // Number of elements to copy
  const size_t num_elems_copy     = num_elems_per_unit * 2;
  // Index to start the copy
  const size_t start_index        = 5;

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  
  // Local range to store copy:
  int local_array[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests local-to-local
  // copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);
  for (int l = 0; l < num_elems_copy; ++l) {
    LOG_MESSAGE("Testing local element %d = %d", l, local_array[l]);
  }
  for (int l = 0; l < num_elems_copy; ++l) {
    ASSERT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
}

TEST_F(CopyTest, BlockingLocalToGlobalSubBlock)
{
  // Copy range of elements contained in a single, continuous block.
}
