
#include "CopyTest.h"

#include <gtest/gtest.h>

#include <dash/Array.h>
#include <dash/Matrix.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/ForEach.h>

#include <dash/pattern/ShiftTilePattern1D.h>
#include <dash/pattern/TilePattern1D.h>
#include <dash/pattern/BlockPattern1D.h>

#include <dash/View.h>

#include <vector>
#include <algorithm>



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
  int * local_copy = new int[num_elem_per_unit];

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  int * dest_end = dash::copy(array.begin(),
                              array.begin() + num_elem_per_unit,
                              local_copy);
  EXPECT_EQ_U(local_copy + num_elem_per_unit, dest_end);
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
    auto lblock         = matrix.local.block(lb);
    auto lblock_view    = lblock.begin().viewspec();
    auto lblock_extents = lblock_view.extents();
    auto lblock_offsets = lblock_view.offsets();
    dash__unused(lblock_offsets);
    EXPECT_EQ_U(block_size_x, lblock_extents[0]);
    EXPECT_EQ_U(block_size_y, lblock_extents[1]);
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
  std::vector<value_t> local_copy(num_elem_per_unit);
  // Pointer to first value in next copy destination range:
  value_t * copy_dest_begin = local_copy.data();
  value_t * copy_dest_last  = local_copy.data();

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
                          std::array<index_t, 2> {{ 0,0 }},
                          g_block_view);
    DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock", " ---- ",
                   "block gidx:", gb,
                   "assigned to unit", g_block_unit.id);
    if (g_block_unit == remote_unit_id) {
      // Block is assigned to selecte remote unit, create local copy:
      auto remote_block_matrix = matrix.block(gb);
      auto remote_block_view   = dash::blocks(matrix)[gb];

      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view:",
                     dash::typestr(remote_block_view));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view",
                     "extents:", remote_block_view.extents(),
                     "offsets:", remote_block_view.offsets(),
                     "size:",    remote_block_view.size());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view domain:",
                     dash::typestr(dash::domain(remote_block_view)));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view origin:",
                     dash::typestr(dash::origin(remote_block_view)));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view domain extents:",
                     dash::domain(remote_block_view).extents());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block view iterator:",
                     dash::typestr(remote_block_view.begin()));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "begin.pos:",  remote_block_view.begin().pos(),
                     "end.pos:",    remote_block_view.end().pos(),
                     "begin.gpos:", remote_block_view.begin().gpos(),
                     "end.gpos:",   remote_block_view.end().gpos());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     dash::test::nview_str(remote_block_view));

      EXPECT_EQ_U(remote_block_matrix.viewspec().offsets(),
                  dash::index(remote_block_view).offsets());
      EXPECT_EQ_U(remote_block_matrix.viewspec().extents(),
                  dash::index(remote_block_view).extents());
#if 0
      copy_dest_last    = dash::copy(remote_block_view.begin(),
                                     remote_block_view.end(),
                                     copy_dest_begin);
#else
      copy_dest_last    = dash::copy(remote_block_view,
                                     copy_dest_begin);
#endif

#if 0
      auto remote_block_range  = dash::make_range(
                                   remote_block_view.begin(),
                                   remote_block_view.end());

      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range:",
                     dash::typestr(remote_block_range));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range",
                     "extents:", remote_block_range.extents(),
                     "offsets:", remote_block_range.offsets(),
                     "size:",    remote_block_range.size());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range domain:",
                     dash::typestr(dash::domain(remote_block_range)));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range origin:",
                     dash::typestr(dash::origin(remote_block_range)));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range domain extents:",
                     dash::domain(remote_block_range).extents());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "source block range iterator:",
                     dash::typestr(remote_block_range.begin()));
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     "begin.pos:",  remote_block_range.begin().pos(),
                     "end.pos:",    remote_block_range.end().pos(),
                     "begin.gpos:", remote_block_range.begin().gpos(),
                     "end.gpos:",   remote_block_range.end().gpos());
      DASH_LOG_DEBUG("CopyTest.Blocking2DimGlobalToLocalBlock",
                     dash::test::nview_str(remote_block_range));

      EXPECT_EQ_U(remote_block_matrix.viewspec().offsets(),
                  dash::index(remote_block_range).offsets());
      EXPECT_EQ_U(remote_block_matrix.viewspec().extents(),
                  dash::index(remote_block_range).extents());

      copy_dest_last    = dash::copy(remote_block_range,
                                     copy_dest_begin);
#endif
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
  for (auto row : local_block_values) {
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

  //
  // Create local copy of first local block (local to local):
  //
  value_t local_block_copy[block_size];
  int  lb      = 0;
  auto l_block = matrix.local.block(lb);
  DASH_LOG_DEBUG("CopyTest.Blocking2Dim", "matrix.local.block(0):",
                 "size:", l_block.size());
  EXPECT_EQ_U(block_size, l_block.size());
  auto local_block_copy_last = dash::copy(l_block.begin(),
                                          l_block.end(),
                                          local_block_copy);
  // Validate number of copied elements:
  auto num_copied = local_block_copy_last - local_block_copy;
  EXPECT_EQ_U(block_size, num_copied);
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
  int * local_copy = new int[num_copy_elem];
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

TEST_F(CopyTest, BlockingLocalToGlobalBlockNDim)
{
  // Copy all elements contained in a single, continuous block.
  const int num_rows_per_unit = 3;
  const int num_cols_per_unit = 7;

  // Distribute row-wise, 3 rows per units:
  dash::SizeSpec<2> sizespec(num_rows_per_unit * dash::size(),
                             num_cols_per_unit);
  dash::DistributionSpec<2> distspec(dash::BLOCKED, dash::NONE);
  dash::TeamSpec<2> teamspec(dash::size(), 1);

  using pattern_t = dash::BlockPattern<2>;

  dash::NArray<int, 2, dash::default_index_t, pattern_t> matrix(
    sizespec, distspec, dash::Team::All(), teamspec);

  std::iota(matrix.local.begin(),
            matrix.local.end(),
            (dash::myid() + 1) * 100);
  matrix.barrier();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                   "initial matrix:");
    DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                   dash::test::nrange_str(matrix));
  }
  matrix.barrier();

  DASH_LOG_DEBUG_VAR("CopyTest.BlockingLocalToGlobalBlockNDim",
                     matrix.local.row(1));
  DASH_LOG_DEBUG_VAR("CopyTest.BlockingLocalToGlobalBlockNDim",
                     matrix.local.row(1).begin());
  DASH_LOG_DEBUG_VAR("CopyTest.BlockingLocalToGlobalBlockNDim",
                     matrix.local.row(1).end());

  DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                 "source range:");
  auto in_range = dash::sub<0>(1, dash::local(matrix));
  DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                 dash::test::nview_str(in_range));

  auto dest_row = ((dash::myid() + 1) * num_rows_per_unit)
                  % matrix.extents()[0];
  DASH_LOG_DEBUG_VAR("CopyTest.BlockingLocalToGlobalBlockNDim",
                     dest_row);

  matrix.barrier();

  // Copy second local row into matrix row at next unit:
  dash::copy(in_range.begin(),
             in_range.end(),
             dash::sub<0>(
               dest_row,
               dest_row + 1,
               matrix).begin()
             );

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                   "result matrix:");
    DASH_LOG_DEBUG("CopyTest.BlockingLocalToGlobalBlockNDim",
                   dash::test::nrange_str(matrix));
  }
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
  int * local_array = new int[num_elems_copy];

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
          return _pattern.unit_at(std::array<index_t, 2> {{ _x, _y }});
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

TEST_F(CopyTest, GlobalToGlobal)
{
  using value_t = int;
  constexpr int elem_per_unit = 100;
  dash::Array<value_t> source(dash::size() * elem_per_unit);
  dash::Array<value_t> target(dash::size() * elem_per_unit);

  dash::fill(target.begin(), target.end(), 0);
  dash::generate_with_index(source.begin(), source.end(),
    [](size_t idx) {
      return dash::myid() * 1000 + idx;
    }
  );

  source.barrier();

  // copy the full range
  dash::copy(source.begin(), source.end(), target.begin());
  source.barrier();

  dash::for_each_with_index(target.begin(), target.end(),
    [](value_t val, size_t idx) {
      ASSERT_EQ_U(val, dash::myid() * 1000 + idx);
    }
  );

  // copy the range with an offset (effectively moving the input
  // range to the left by 1)
  dash::copy(source.begin() + 1, source.end(), target.begin());
  source.barrier();

  dash::for_each_with_index(target.begin(), target.end() - 1,
    [](value_t val, size_t idx) {
      std::cout << idx << ": " << val << std::endl;
      // the array has shifted so the last element is different
      if ((idx % elem_per_unit) == (elem_per_unit - 1)) {
        // the last element comes from the next unit
        // this element has not been copied on the last unit
        ASSERT_EQ_U(val, (dash::myid() + 1) * 1000 + idx + 1);
      } else {
        ASSERT_EQ_U(val, dash::myid() * 1000 + idx + 1);
      }
    }
  );
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

#if 0
// TODO
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
  std::vector<int> local_vector;

  // Copy values from global range to local memory.
  // All units copy first block, so unit 0 tests local-to-local copying.
  auto future = dash::copy_async(array.begin(),
                                   array.end(),
                                   local_vector.begin());
  auto local_dest_end = future.get();

  EXPECT_EQ_U(num_elem_total, local_dest_end - local_vector.begin());
  for (size_t i = 0; i < array.size(); ++i) {
    EXPECT_EQ_U(static_cast<int>(array[i]), local_vector[i]);
  }
}
#endif
