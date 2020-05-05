
#include <gtest/gtest.h>

#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/algorithm/Fill.h>

#include <dash/algorithm/Copy.h>
#include <dash/pattern/BlockPattern1D.h>
#include <dash/pattern/ShiftTilePattern1D.h>
#include <dash/pattern/TilePattern1D.h>

#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "CopyTest.h"

#include <vector>


TEST_F(CopyTest, BlockingGlobalToLocalBlock)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid().id + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  auto* local_copy = new int[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  int * dest_end = dash::copy(array.begin(),
                              array.begin() + num_elem_per_unit,
                              local_copy);
  auto val = local_copy + num_elem_per_unit;

  EXPECT_EQ_U(val, dest_end);
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    EXPECT_EQ_U(static_cast<int>(array[l]),
                local_copy[l]);
  }
  delete[] local_copy;
}

TEST_F(CopyTest, Blocking2DimGlobalToLocalBlock)
{
  // Copy all blocks from a single remote unit.
  const size_t block_size_x  = 3;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t num_local_blocks_x  = 2;
  size_t num_local_blocks_y  = 2;
  size_t num_blocks_x        = _dash_size * num_local_blocks_x;
  size_t num_blocks_y        = _dash_size * num_local_blocks_y;
  size_t num_blocks_total    = num_blocks_x * num_blocks_y;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / _dash_size;
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;

  if (_dash_size < 2) {
    LOG_MESSAGE("CopyTest.Blocking2DimGlobalToLocalBlock "
                "requires > 1 units");
    return;
  }

  LOG_MESSAGE("nunits:%zu elem_total:%zu "
              "elem_per_unit:%zu blocks_per_unit:%zu",
              _dash_size, num_elem_total,
              num_elem_per_unit, num_blocks_per_unit);

  typedef dash::ShiftTilePattern<2>      pattern_t;
  typedef typename pattern_t::index_type index_t;
  typedef float                          value_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_x,
      extent_y),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y))
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix(pattern);

  // Assign initial values:
  for (size_t lb = 0; lb < num_blocks_per_unit; ++lb) {
    LOG_MESSAGE("initialize values in local block %zu", lb);
    auto lblock         = matrix.local.block(lb);
    auto lblock_view    = lblock.begin().viewspec();
    auto lblock_extents = lblock_view.extents();
    auto lblock_offsets = lblock_view.offsets();
    dash__unused(lblock_offsets);
    EXPECT_EQ_U(block_size_x, lblock_extents[0]);
    EXPECT_EQ_U(block_size_y, lblock_extents[1]);
    LOG_MESSAGE("local block %zu offset: (%li,%li) extent: (%lu,%lu)",
                lb,
                lblock_offsets[0], lblock_offsets[1],
                lblock_extents[0], lblock_extents[1]);
    for (auto bx = 0; bx < static_cast<int>(lblock_extents[0]); ++bx) {
      for (auto by = 0; by < static_cast<int>(lblock_extents[1]); ++by) {
        // Phase coordinates (bx,by) to global coordinates (gx,gy):
        index_t gx       = lblock_view.offset(0) + bx;
        index_t gy       = lblock_view.offset(1) + by;
        dash__unused(gx);
        dash__unused(gy);
        value_t value    = static_cast<value_t>(dash::myid().id + 1) +
                           (0.00001 * (
                             ((lb + 1) * 10000) +
                             ((bx + 1) * 100) +
                             by + 1
                           ));
        LOG_MESSAGE("set local block %zu at phase:(%d,%d) g:(%li,%li) = %f",
                    lb, bx, by, gx, gy, value);
        lblock[bx][by] = value;
      }
    }
  }

  matrix.barrier();

  // Log matrix values:
  if (dash::myid().id == 0) {
    std::vector< std::vector<value_t> > matrix_values;
    for (size_t x = 0; x < extent_x; ++x) {
      std::vector<value_t> row;
      for (size_t y = 0; y < extent_y; ++y) {
        DASH_LOG_DEBUG("CopyTest.Blocking2Dim", "get matrix value at",
                       "x:", x,
                       "y:", y);
        value_t value = matrix[x][y];
        row.push_back(value);
      }
      matrix_values.push_back(row);
    }
    for (size_t row = 0; row < extent_x; ++row) {
      DASH_LOG_DEBUG_VAR("CopyTest.Blocking2Dim", matrix_values[row]);
    }
  }

  matrix.barrier();

  // Array to store local copy:
  auto* local_copy = new value_t[num_elem_per_unit];
  // Pointer to first value in next copy destination range:
  value_t * copy_dest_begin = local_copy;
  value_t * copy_dest_last  = local_copy;

  //
  // Create local copy of all blocks from a single remote unit:
  //
  dash::team_unit_t remote_unit_id(
                       (dash::Team::All().myid().id + 1) % _dash_size);
  LOG_MESSAGE("Creating local copy of blocks at remote unit %d",
              remote_unit_id.id);
  int rb = 0;
  for (size_t gb = 0; gb < num_blocks_total; ++gb) {
    // View of block at global block index gb:
    auto g_block_view = pattern.block(gb);
    // Unit assigned to block at global block index gb:
    auto g_block_unit = pattern.unit_at(
                          std::array<index_t, 2> {0,0},
                          g_block_view);
    LOG_MESSAGE("Block %zu: assigned to unit %d", gb, g_block_unit.id);
    if (g_block_unit == remote_unit_id) {
      // Block is assigned to selecte remote unit, create local copy:
      LOG_MESSAGE("Creating local copy of block %zu", gb);
      auto remote_block      = matrix.block(gb);
      auto remote_block_view = remote_block.begin().viewspec();
      dash__unused(remote_block_view);
      LOG_MESSAGE("Block %zu index range: (%li..%li] "
                  "offset: (%li,%li) extent: (%lu,%lu)",
                  gb, remote_block.begin().pos(), remote_block.end().pos(),
                  remote_block_view.offset(0), remote_block_view.offset(1),
                  remote_block_view.extent(0), remote_block_view.extent(1));
      copy_dest_last = dash::copy(remote_block.begin(),
                                  remote_block.end(),
                                  copy_dest_begin);
      // Validate number of copied elements:
      auto num_copied = copy_dest_last - copy_dest_begin;
      EXPECT_EQ_U(num_copied, block_size);
      // Advance local copy destination pointer:
      copy_dest_begin = copy_dest_last;
      ++rb;
    }
  }
  // Validate number of copied blocks:
  EXPECT_EQ_U(num_blocks_per_unit, rb);

  // Log values in local copy:
  std::vector< std::vector<value_t> > local_block_values;
  for (size_t lb = 0; lb < num_blocks_per_unit; ++lb) {
    for (size_t bx = 0; bx < block_size_x; ++bx) {
      std::vector<value_t> row;
      for (size_t by = 0; by < block_size_y; ++by) {
        auto l_offset = (lb * block_size) + (bx * block_size_y) + by;
        auto value    = local_copy[l_offset];
        row.push_back(value);
      }
      local_block_values.push_back(row);
    }
  }
  for (const auto& row : local_block_values) {
    DASH_LOG_DEBUG_VAR("CopyTest.Blocking2Dim", row);
  }

  // Validate values:
  for (size_t lb = 0; lb < num_blocks_per_unit; ++lb) {
    for (size_t bx = 0; bx < block_size_x; ++bx) {
      for (size_t by = 0; by < block_size_y; ++by) {
        auto    l_offset = (lb * block_size) + (bx * block_size_y) + by;
        value_t expected = static_cast<value_t>(remote_unit_id + 1) +
                           (0.00001 * (
                             ((lb + 1) * 10000) +
                             ((bx + 1) * 100) +
                             by + 1
                           ));
        EXPECT_EQ_U(expected, local_copy[l_offset]);
      }
    }
  }
  delete[] local_copy;

  //
  // Create local copy of first local block (local to local):
  //
  value_t local_block_copy[block_size];
  int  lb      = 0;
  auto l_block = matrix.local.block(lb);
  LOG_MESSAGE("Creating local copy of first local block");
  value_t * local_block_copy_last =
    dash::copy(l_block.begin(),
               l_block.end(),
               local_block_copy);
  // Validate number of copied elements:
  auto num_copied = local_block_copy_last - local_block_copy;
  EXPECT_EQ_U(num_copied, block_size);
  for (size_t bx = 0; bx < block_size_x; ++bx) {
    for (size_t by = 0; by < block_size_y; ++by) {
      auto    l_offset = (bx * block_size_y) + by;
      value_t expected = static_cast<value_t>(dash::myid().id + 1) +
                         (0.00001 * (
                           ((lb + 1) * 10000) +
                           ((bx + 1) * 100) +
                           by + 1
                         ));
      EXPECT_EQ_U(expected, local_block_copy[l_offset]);
    }
  }
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

  LOG_MESSAGE("lstart:%li lend:%li ncopy:%zu",
              l_start_idx, l_end_idx, num_copy_elem);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (int l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid().id + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  auto* local_copy = new int[num_copy_elem];
  int * dest_first = local_copy;
  int * dest_last  = local_copy;
  if (dash::myid().id == 0) {
    // Copy elements in front of local range:
    LOG_MESSAGE("Copying from global range (%d-%ld]",
                0, l_start_idx);
    dest_first = dash::copy(
                   array.begin(),
                   array.begin() + l_start_idx,
                   dest_first);
    // Copy elements after local range:
    LOG_MESSAGE("Copying from global range (%li-%zu]",
                l_end_idx, array.size());
    dest_last  = dash::copy(
                   array.begin() + l_end_idx,
                   array.end(),
                   dest_first);
    dash__unused(dest_last);
    LOG_MESSAGE("Validating elements");
    int l = 0;
    for (size_t g = 0; g < array.size(); ++g) {
      if (array.pattern().unit_at(g) != dash::myid().id) {
        int expected = array[g];
        EXPECT_EQ_U(expected, local_copy[l]);
        ++l;
      }
    }
  }
  delete[] local_copy;
}

TEST_F(CopyTest, Blocking2DimGlobalToLocalMasterOnlyAllRemote)
{
  using value_t  = double;
  using Pattern_t = dash::Pattern<2>;
  using Matrix_t  = dash::Matrix<value_t, 2, typename Pattern_t::index_type, Pattern_t>;

  dash::global_unit_t myid = dash::myid();
  size_t num_units = dash::Team::All().size();
  size_t num_elems_unit  = 5;
  size_t extent          = num_elems_unit * num_units;

  dash::TeamSpec<2> teamspec{};
  auto distspec = dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE);
  Pattern_t pattern(dash::SizeSpec<2>(extent, extent), distspec,
                  teamspec, dash::Team::All());
  Matrix_t matrix(pattern);

  auto& local = matrix.local;   // local matrix
  for (auto i = 0; i < local.extent(0); ++i) {
      for (auto j = 0; j < local.extent(1); ++j) {
          local.at(i,j) = i + j + dash::myid() / 100.0;
      }
  }
  dash::barrier();

  std::vector<value_t> copy_buffer(extent);

  if (!myid) {
    for (auto col_id = 0; col_id < matrix.extent(1); ++col_id) {

      auto col_it = matrix.col(col_id);
      dash::copy(col_it.begin(), col_it.end(), copy_buffer.data());
      size_t count = 0;
      for (auto it = col_it.begin(); it != col_it.end(); ++it, ++count) {
        EXPECT_EQ_U(static_cast<value_t>(*it), copy_buffer[count]);
      }
    }
  }

  dash::barrier();
}

TEST_F(CopyTest, BlockingGlobalToLocalBarrierUnaligned)
{
  dash::global_unit_t myid = dash::myid();
  size_t num_units       = dash::Team::All().size();
  size_t num_elems_unit  = 20;
  size_t start_index     = 7;
  size_t num_elems_copy  = num_elems_unit;
  size_t num_elems_total = num_elems_unit * num_units;

  if (dash::size() < 2) {
    num_elems_copy = num_elems_unit - start_index - 1;
  }

  std::vector<int> local_array(num_elems_copy);
  dash::Array<int> array(num_elems_total);

  LOG_MESSAGE("Elements per unit: %zu", num_elems_unit);
  LOG_MESSAGE("Start index:       %zu", start_index);
  LOG_MESSAGE("Elements to copy:  %zu", num_elems_copy);
  LOG_MESSAGE("Array size:        %zu", array.size());

  std::fill(array.lbegin(), array.lend(), myid);

  array.barrier();

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array.data());

  array.barrier();

  for (size_t l = 0; l < num_elems_copy; ++l) {
    EXPECT_EQ_U(local_array[l],
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
  int target_range[num_elem_per_unit];

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 10000) + (l * 10);
    local_range[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Block- and global offset of target range:
  auto block_offset  = _dash_size - 1 - dash::myid();
  auto global_offset = block_offset * num_elem_per_unit;

  // First, create local copy of remote target region and check
  // its initial values:
  dash::copy(array.begin() + global_offset,
             array.begin() + global_offset + num_elem_per_unit,
             target_range);

  for (auto l = 0; l < num_elem_per_unit; ++l) {
    int target_unit_id = _dash_size - 1 - dash::myid();
    int expected_value = ((target_unit_id + 1) * 10000) + (l * 10);
    // Test values when obtained from dash::copy:
    EXPECT_EQ_U(expected_value,
                target_range[l]);
    // Test values when obtained from single dart_get requests:
    EXPECT_EQ_U(expected_value,
                static_cast<int>(array[global_offset + l]));
  }
  array.barrier();

  // Copy values from local range to remote global range.
  // All units (u) copy into block (nblocks-1-u), so unit 0 copies into
  // last block.
  dash::copy(local_range,
             local_range + num_elem_per_unit,
             array.begin() + global_offset);

  array.barrier();

  for (auto l = 0; l < num_elem_per_unit; ++l) {
    EXPECT_EQ_U(local_range[l],
                static_cast<int>(array[global_offset + l]));
  }

  array.barrier();
}

TEST_F(CopyTest, AsyncLocalToGlobPtrWait)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 5;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  // Global target range:
  dash::Array<int> array(num_elem_total, dash::BLOCKED);
  // Local range to copy:
  int local_range[num_elem_per_unit];

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 110000) + l;
    local_range[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Copy values from local range to remote global range.
  // All units (u) copy into block (nblocks-1-u), so unit 0 copies into
  // last block.
  auto block_offset  = (dash::myid() + 1) % dash::size();
  auto global_offset = block_offset * num_elem_per_unit;

  using glob_it_t    = decltype(array.begin());
  using glob_ptr_t   = typename glob_it_t::pointer;

  glob_ptr_t gptr_dest = static_cast<glob_ptr_t>(
                           array.begin() + global_offset);
  LOG_MESSAGE("CopyTest.AsyncLocalToGlobPtrWait: call copy_async");

  auto copy_fut = dash::copy_async(local_range,
                                   local_range + num_elem_per_unit,
                                   gptr_dest);

  // Blocks until remote completion:
  LOG_MESSAGE("CopyTest.AsyncLocalToGlobPtrWait: call fut.wait");
  copy_fut.wait();

  array.barrier();

  for (auto l = 0; l < num_elem_per_unit; ++l) {
    // Compare local buffer and global array dest range:
    EXPECT_EQ_U(local_range[l],
                static_cast<int>(array[global_offset + l]));
  }
  array.barrier();
}


TEST_F(CopyTest, AsyncLocalToGlobPtrTest)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 5;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  // Global target range:
  dash::Array<int> array(num_elem_total, dash::BLOCKED);
  // Local range to copy:
  int local_range[num_elem_per_unit];

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 110000) + l;
    local_range[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Copy values from local range to remote global range.
  // All units (u) copy into block (nblocks-1-u), so unit 0 copies into
  // last block.
  auto block_offset  = (dash::myid() + 1) % dash::size();
  auto global_offset = block_offset * num_elem_per_unit;

  using glob_it_t    = decltype(array.begin());
  using glob_ptr_t   = typename glob_it_t::pointer;

  glob_ptr_t gptr_dest = static_cast<glob_ptr_t>(
                           array.begin() + global_offset);
  LOG_MESSAGE("CopyTest.AsyncLocalToGlobPtrTest: call copy_async");

  auto copy_fut = dash::copy_async(local_range,
                                   local_range + num_elem_per_unit,
                                   gptr_dest);

  // Blocks until remote completion:
  LOG_MESSAGE("CopyTest.AsyncLocalToGlobPtrTest: call fut.test");
  while (!copy_fut.test()) {}

  array.barrier();

  for (auto l = 0; l < num_elem_per_unit; ++l) {
    // Compare local buffer and global array dest range:
    EXPECT_EQ_U(local_range[l],
                static_cast<int>(array[global_offset + l]));
  }
  array.barrier();
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
  for (size_t l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  LOG_MESSAGE("Waiting for barrier");
  array.barrier();

  // Local range to store copy:
  int local_array[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests
  // local-to-local copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);

  LOG_MESSAGE("Waiting for barrier");
  array.barrier();

  for (size_t l = 0; l < num_elems_copy; ++l) {
    LOG_MESSAGE("Testing local value %zu", l);
    EXPECT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalSubBlockTwoUnits)
{
  // Copy all elements contained in a single, continuous block,
  // starting from an index unequal 0.

  if (_dash_size < 2) {
    return;
  }

  const size_t num_elems_per_unit = 20;
  const size_t num_elems_total    = _dash_size * num_elems_per_unit;
  // Number of elements to copy
  const size_t num_elems_copy     = num_elems_per_unit;
  // Index to start the copy
  const size_t start_index        = 5;

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (size_t l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  int local_array[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests
  // local-to-local copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);
  for (size_t l = 0; l < num_elems_copy; ++l) {
    EXPECT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
}

TEST_F(CopyTest, BlockingGlobalToLocalSubBlockThreeUnits)
{
  // Copy all elements contained in a single, continuous block,
  // starting from an index unequal 0.

  if(_dash_size < 3) {
    LOG_MESSAGE("CopyTest.BlockingGlobalToLocalSubBlockThreeUnits "
                "requires > 3 units");
    return;
  }

  const size_t num_elems_per_unit = 20;
  const size_t num_elems_total    = _dash_size * num_elems_per_unit;
  // Number of elements to copy
  const size_t num_elems_copy     = num_elems_per_unit * 2;
  // Index to start the copy
  const size_t start_index        = 5;

  dash::Array<int> array(num_elems_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (size_t l = 0; l < num_elems_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  auto* local_array = new int[num_elems_copy];

  // Copy values from global range to local memory.
  // All units copy a part of the first block, so unit 0 tests
  // local-to-local copying.
  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);
  for (size_t l = 0; l < num_elems_copy; ++l) {
    EXPECT_EQ_U(static_cast<int>(array[l+start_index]), local_array[l]);
  }
  delete[] local_array;
}

TEST_F(CopyTest, AsyncGlobalToLocalTiles)
{
  typedef double
    value_t;
  typedef dash::TilePattern<2>
    pattern_t;
  typedef dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix_t;
  typedef typename pattern_t::index_type
    index_t;

  if (_dash_size < 3) {
    LOG_MESSAGE("CopyTest.AsyncGlobalToLocalTiles requires > 3 units");
    return;
  }
  if (_dash_size % 2 != 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for "
                "CopyTest.AsyncGlobalToLocalTiles");
    return;
  }

  size_t tilesize_x     = 2;
  size_t tilesize_y     = 3;
  size_t num_block_elem = tilesize_x * tilesize_y;
  // Additional blocks in both dimensions to ensure unbalanced mapping:
  size_t odd_blocks_x   = std::ceil(sqrt(_dash_size)) + 1;
  size_t odd_blocks_y   = 1;
  size_t num_blocks_x   = (_dash_size / 2 + odd_blocks_x);
  size_t num_blocks_y   = (_dash_size / 2 + odd_blocks_y);
  size_t extent_x       = num_blocks_x * tilesize_x;
  size_t extent_y       = num_blocks_y * tilesize_y;

  dash::SizeSpec<2> sizespec(extent_x, extent_y);
  dash::DistributionSpec<2> distspec(dash::TILE(tilesize_x),
                                     dash::TILE(tilesize_y));
  dash::TeamSpec<2> teamspec;
  teamspec.balance_extents();

  LOG_MESSAGE("SizeSpec(%lu,%lu) TeamSpec(%lu,%lu)",
              sizespec.extent(0), sizespec.extent(1),
              teamspec.extent(0), teamspec.extent(1));

  pattern_t pattern(sizespec, distspec, teamspec);

  if (_dash_id == 0) {
    dash::test::print_pattern_mapping(
      "matrix.pattern.unit_at", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> dart_unit_t {
          return _pattern.unit_at(
                   std::array<index_t, 2> {{ _x, _y }});
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.at", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _pattern.at(
                   std::array<index_t, 2> {{ _x, _y }});
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.block_at", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _pattern.block_at(
                   std::array<index_t, 2> {{ _x, _y }});
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.block.offset", pattern, 5,
      [](const pattern_t & _pattern, int _x, int _y) -> std::string {
          auto block_idx = _pattern.block_at(
                             std::array<index_t, 2> {{ _x, _y }});
          auto block_vs  = _pattern.block(block_idx);
          std::ostringstream ss;
          ss << block_vs.offset(0) << "," << block_vs.offset(1);
          return ss.str();
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.local_index", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _pattern.local_index(
                   std::array<index_t, 2> {{ _x, _y }}).index;
      });
  }

  matrix_t matrix_a(pattern);
  matrix_t matrix_b(pattern);

  auto lblockspec_a = matrix_a.pattern().local_blockspec();
  auto lblockspec_b = matrix_b.pattern().local_blockspec();
  auto blockspec_a  = matrix_a.pattern().blockspec();

  size_t num_local_blocks_a = lblockspec_a.size();
  size_t num_local_blocks_b = lblockspec_b.size();

  EXPECT_EQ_U(num_local_blocks_a, num_local_blocks_b);

  LOG_MESSAGE("lblockspec_a(%lu,%lu)[%zu] lblockspec_b(%lu,%lu)[%zu]",
              lblockspec_a.extent(0), lblockspec_a.extent(1),
              num_local_blocks_a,
              lblockspec_b.extent(0), lblockspec_b.extent(1),
              num_local_blocks_b);

  // Initialize values in local blocks of matrix A:
  for (int lb = 0; lb < static_cast<int>(num_local_blocks_a); ++lb) {
    auto lblock = matrix_a.local.block(lb);
    for (auto lit = lblock.begin(); lit != lblock.end(); ++lit) {
      *lit = dash::myid() + 0.1 * lb + 0.01 * lit.pos();
    }
  }

  matrix_a.barrier();

  if (_dash_id == 0) {
    dash::test::print_pattern_mapping(
      "matrix.a", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> dart_unit_t {
          return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
      });
    dash::test::print_matrix("matrix.a", matrix_a, 2);
  }

  // Copy blocks of matrix A from neighbor unit into local blocks of
  // matrix B:

  // Request handles from asynchronous copy operations:
  std::vector< dash::Future<value_t*> > req_handles;
  // Local copy target pointers for later validation:
  std::vector< value_t* >               dst_pointers;
  for (int lb = 0; lb < static_cast<int>(num_local_blocks_a); ++lb) {
    // Get native pointer of local block of B as destination of copy:
    auto matrix_b_lblock   = matrix_b.local.block(lb);
    auto matrix_b_dest     = matrix_b_lblock.begin().local();
    static_assert(std::is_same<decltype(matrix_b_dest), value_t*>::value, "must be true");
    auto lblock_b_offset_x = matrix_b_lblock.offset(0);
    auto lblock_b_offset_y = matrix_b_lblock.offset(1);
    auto lblock_b_gcoord_x = lblock_b_offset_x / tilesize_x;
    auto lblock_b_gcoord_y = lblock_b_offset_y / tilesize_y;
    auto block_a_gcoord_x  = (lblock_b_gcoord_x + 1) % num_blocks_x;
    auto block_a_gcoord_y  = (lblock_b_gcoord_y + 1) % num_blocks_y;
    auto block_a_index     = blockspec_a.at(block_a_gcoord_x,
                                            block_a_gcoord_y);
    auto gblock_a          = matrix_a.block(block_a_index);

    LOG_MESSAGE("local block %d: copy_async: "
                "A.block((%lu,%lu):%lu) -> B.block((%lu,%lu):%d)",
                lb,
                block_a_gcoord_x,  block_a_gcoord_y,  block_a_index,
                lblock_b_gcoord_x, lblock_b_gcoord_y, lb);

    EXPECT_NE_U(nullptr, matrix_b_dest);
    auto req = dash::copy_async(gblock_a.begin(),
                                gblock_a.end(),
                                matrix_b_dest);
    req_handles.push_back(std::move(req));
    dst_pointers.push_back(matrix_b_dest);
  }

  // Create some CPU load
  double m = 123.10;
  double n = 234.23;
  double p = 322.12;
  for (size_t i = 0; i < 50000000; ++i) {
    m = (n / std::pow(p, 1.0/3.0)) + sqrt(m);
  }
  // To prevent compiler from removing work load loop in optimization:
  LOG_MESSAGE("Dummy result: %f", m);

  for (auto& req : req_handles) {
    // Wait for completion of async copy operation.
    // Returns pointer to final element copied into target range:
    value_t * copy_dest_end   = req.get();
    // Corresponding pointer to start of copy target range, also tests
    // number of elements copied:
    value_t * copy_dest_begin = copy_dest_end - num_block_elem;
    // Test if correspondig start pointer is in set of start pointers used
    // for copy_async:
    EXPECT_TRUE_U(std::find(dst_pointers.begin(), dst_pointers.end(),
                            copy_dest_begin) != dst_pointers.end());
  }

  // Wait for all units to complete their copy operations:
  matrix_a.barrier();

  if (_dash_id == 0) {
    dash::test::print_matrix("matrix.b", matrix_b, 2);
  }

  // Validate copied values:
  for (int lb = 0; lb < static_cast<int>(num_local_blocks_a); ++lb) {
//  auto block_view = matrix_a.local.block(lb);
  }
}

TEST_F(CopyTest, AsyncGlobalToLocalBlockWait)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  EXPECT_EQ_U(num_elem_per_unit, array.local.size());
  EXPECT_EQ_U(num_elem_per_unit, array.lsize());

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  int local_copy[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  auto dest_end = dash::copy_async(array.begin(),
                                   array.begin() + num_elem_per_unit,
                                   local_copy);
  dest_end.wait();

  EXPECT_EQ_U(num_elem_per_unit, dest_end.get() - local_copy);
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    EXPECT_EQ_U(static_cast<int>(array[l]),
                local_copy[l]);
  }
}

TEST_F(CopyTest, AsyncGlobalToLocalTest)
{
  // Copy all elements contained in a single, continuous block.
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  EXPECT_EQ_U(num_elem_per_unit, array.local.size());
  EXPECT_EQ_U(num_elem_per_unit, array.lsize());

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Local range to store copy:
  int local_copy[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  auto dest_end = dash::copy_async(array.begin(),
                                   array.begin() + num_elem_per_unit,
                                   local_copy);

  // spin until the transfer is completed
  while (!dest_end.test()) { }

  EXPECT_EQ_U(num_elem_per_unit, dest_end.get() - local_copy);
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    EXPECT_EQ_U(static_cast<int>(array[l]),
                local_copy[l]);
  }
}

TEST_F(CopyTest, AsyncAllToLocalVector)
{
  // Copy all elements of global array into local vector:
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  // Local vector to store copy of global array;
  int *local_copy = new int[num_elem_per_unit*dash::size()];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  auto future = dash::copy_async(array.begin(),
                                 array.end(),
                                 local_copy);
  auto local_dest_end = future.get();

  EXPECT_EQ_U(num_elem_total, local_dest_end - local_copy);
  for (size_t i = 0; i < array.size(); ++i) {
    EXPECT_EQ_U(static_cast<int>(array[i]), local_copy[i]);
  }
  delete[] local_copy;
}

TEST_F(CopyTest, AsyncLocalVectorToAll)
{
  // Copy all elements of global array into local vector:
  const int num_elem_per_unit = 20;
  size_t num_elem_total       = _dash_size * num_elem_per_unit;

  dash::Array<int> array(num_elem_total, dash::BLOCKED);

  // Unit 0 copies values into the whole global array
  if (dash::myid() == 0) {
    int *local_copy = new int[num_elem_total];

    // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
    for (int unit = 0; unit < dash::size(); ++unit) {
      for (auto l = 0; l < num_elem_per_unit; ++l) {
        local_copy[unit*num_elem_per_unit + l] = ((unit + 1) * 1000) + l;
      }
    }

    auto future = dash::copy_async(local_copy,
                                  local_copy + num_elem_total,
                                  array.begin());

    auto global_dest_end = future.get();
    EXPECT_EQ_U(num_elem_total, dash::distance(array.begin(), global_dest_end));
    delete[] local_copy;
  }
  array.barrier();

  // All units check for proper values on their side
  for (auto l = 0; l < num_elem_per_unit; ++l) {
    EXPECT_EQ_U(array.local[l], ((dash::myid() + 1) * 1000) + l);
  }
}

TEST_F(CopyTest, MatrixToSmallerTeam)
{
  if (_dash_size < 2) {
    SKIP_TEST_MSG("At least 2 units required for this test.");
  }

  using TeamSpecT = dash::TeamSpec<2>;
  using MatrixT = dash::NArray<double, 2>;
  using PatternT = typename MatrixT::pattern_type;
  using SizeSpecT = dash::SizeSpec<2>;
  using DistSpecT = dash::DistributionSpec<2>;

  auto& team_all = dash::Team::All();
  TeamSpecT team_all_spec(team_all.size(), 1);
  team_all_spec.balance_extents();

  auto size_spec = SizeSpecT(4*team_all_spec.extent(1),
                             4*team_all_spec.extent(1));
  auto dist_spec = DistSpecT(dash::BLOCKED, dash::BLOCKED);

  MatrixT grid_more(size_spec, dist_spec, team_all, team_all_spec);
  dash::fill(grid_more.begin(), grid_more.end(), (double)team_all.myid());
  team_all.barrier();

  // create a smaller team
  dash::Team& team_fewer= team_all.split(2);
  team_all.barrier();
  if (!team_fewer.is_null() && 0 == team_fewer.position()) {
    TeamSpecT team_fewer_spec(team_fewer.size(), 1);
    team_fewer_spec.balance_extents();

    MatrixT grid_fewer(size_spec, dist_spec, team_fewer, team_fewer_spec);
    dash::fill(grid_fewer.begin(), grid_fewer.end(), -1.0);

    auto lextents= grid_fewer.pattern().local_extents();

    for (uint32_t y = 0; y < lextents[0]; ++y) {
      auto gcorner_fewer = grid_fewer.pattern().global({y, 0});
      auto gbegin = grid_more.begin() + grid_more.pattern().global_at(gcorner_fewer);

      auto loffset = grid_fewer.pattern().local_at({y, 0});
      dash::copy(gbegin, gbegin + lextents[1],
                 grid_fewer.lbegin() + loffset);
    }
    team_fewer.barrier();

    if (team_fewer.myid() == 0) {
      auto gextents = grid_fewer.extents();
      for (uint32_t y = 0; y < gextents[0]; ++y) {
        for (uint32_t x = 0; x < gextents[1]; ++x) {
          ASSERT_EQ_U(grid_more(y, x), grid_fewer(y, x));
        }
      }
    }

    team_fewer.barrier();

  }
}

TEST_F(CopyTest, InputOutputTypeTest)
{
  /* signed/unsigned and const conversion is permitted */
  ASSERT_TRUE_U((dash::internal::is_dash_copyable<int, int>::value));
  ASSERT_TRUE_U((dash::internal::is_dash_copyable<const int, int>::value));
  ASSERT_TRUE_U((dash::internal::is_dash_copyable<const int, unsigned int>::value));
  ASSERT_TRUE_U((dash::internal::is_dash_copyable<float, float>::value));
  /* size conversion is not permitted */
  ASSERT_FALSE_U((dash::internal::is_dash_copyable<uint64_t, uint32_t>::value));
  ASSERT_FALSE_U((dash::internal::is_dash_copyable<float, double>::value));

  struct point_t {
    int a;
    int b;
  };
  /* no conversion between arithmetic types and non-arithmetic types */
  ASSERT_FALSE_U((dash::internal::is_dash_copyable<point_t, uint64_t>::value));
  ASSERT_TRUE_U((dash::internal::is_dash_copyable<const point_t, point_t>::value));

}
