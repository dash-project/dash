#include "TestBase.h"
#include <libdash.h>
#include "PatternTest.h"
#include <dash/internal/Math.h>

TEST_F(PatternTest, SimpleConstructor) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 21;
  int extent_y = 37;
  int extent_z = 41;
  int size = extent_x * extent_y * extent_z;
  // Should default to distribution BLOCKED, NONE, NONE:
  dash::Pattern<3> pat_default(extent_x, extent_y, extent_z);
  EXPECT_EQ(dash::DistributionSpec<3>(), pat_default.distspec());
  EXPECT_EQ(dash::Team::All(), pat_default.team());
  EXPECT_EQ(dash::Team::All().size(), pat_default.num_units());
  EXPECT_EQ(size, pat_default.capacity());

  dash::DistributionSpec<3> ds_blocked_z(
      dash::NONE, dash::NONE, dash::BLOCKED);
  dash::Pattern<3> pat_ds(
      extent_x, extent_y, extent_z, 
      ds_blocked_z);
  EXPECT_EQ(ds_blocked_z, pat_ds.distspec());
  EXPECT_EQ(size, pat_ds.capacity());

  // Splits in consecutive test cases within a single test
  // run are not supported for now:
  // dash::Team & team_split_2 = dash::Team::All().split(2);
  dash::Pattern<3> pat_ds_t(
      extent_x, extent_y, extent_z, 
      ds_blocked_z,
      dash::Team::All());
  EXPECT_EQ(ds_blocked_z, pat_ds_t.distspec());
  EXPECT_EQ(size, pat_ds_t.capacity());
  EXPECT_EQ(dash::Team::All().size(), pat_ds_t.num_units());
}

TEST_F(PatternTest, EqualityComparison) {
  DASH_TEST_LOCAL_ONLY();
  int num_units = dash::Team::All().size();
  int extent_x = 21;
  int extent_y = 37;
  int extent_z = 41;
  dash::Pattern<3> pat_1(extent_x, extent_y, extent_z);
  dash::Pattern<3> pat_2(extent_x, extent_y + 1, extent_z);
  dash::Pattern<3> pat_3(extent_x, extent_y + 1, extent_z,
                         dash::NONE, dash::BLOCKED, dash::NONE);
  dash::Pattern<3> pat_4(extent_x, extent_y + 1, extent_z,
                         dash::TeamSpec<3>(1,num_units,1));
  dash::Pattern<3> pat_5(extent_x, extent_y, extent_z,
                         dash::TeamSpec<3>(num_units,1,1));
  EXPECT_EQ(pat_1, pat_1);
  EXPECT_EQ(pat_1, pat_5);
  EXPECT_NE(pat_1, pat_2);
  EXPECT_NE(pat_1, pat_3);
  EXPECT_NE(pat_1, pat_4);
}

TEST_F(PatternTest, CopyConstructorAndAssignment) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 12;
  int extent_y = 13;
  int extent_z = 14;
  // Splits in consecutive test cases within a single test
  // run are not supported for now:
  // dash::Team & team_split_2 = dash::Team::All().split(2);
  int num_units = dash::Team::All().size();
  if (num_units % 2 == 0) {
    // This test requires that (2 * 1 * (num_units/2)) == num_units
    dash::TeamSpec<3> teamspec_2_by_n(2, 1, num_units / 2);
    dash::Pattern<3> pat_org(
        dash::SizeSpec<3>(3, 7, 13),
        dash::DistributionSpec<3>(dash::BLOCKED, dash::NONE, dash::CYCLIC),
        teamspec_2_by_n,
        dash::Team::All());

    dash::Pattern<3> pat_copy(pat_org);
    dash::Pattern<3> pat_assign(extent_x, extent_y, extent_z);
    pat_assign = pat_org;
  }
}

TEST_F(PatternTest, Distribute1DimBlocked) {
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional blocked partitioning:
  //
  // [ .. team 0 .. | .. team 1 .. | ... | team n-1 ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = dash::math::div_ceil(_num_elem, team_size);
  size_t local_cap  = block_size;
  dash::Pattern<1, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  dash::Pattern<1, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKED),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size);
  EXPECT_EQ(pat_blocked_row.max_elem_per_unit(), local_cap);
  EXPECT_EQ(pat_blocked_col.capacity(), _num_elem);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size);
  EXPECT_EQ(pat_blocked_col.max_elem_per_unit(), local_cap);
  
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int expected_unit_id = x / block_size;
    int expected_offset  = x % block_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    LOG_MESSAGE("x: %d, eu: %d, eo: %d",
      x, expected_unit_id, expected_offset);
    // Row order:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_row.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_row.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_blocked_row.local_to_global_index(
        expected_unit_id, expected_offset));
    // Column order:
    EXPECT_EQ(
      expected_coords,
      pat_blocked_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blocked_col.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blocked_col.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_blocked_col.local_to_global_index(
        expected_unit_id, expected_offset));
  }
}

TEST_F(PatternTest, Distribute1DimCyclic) {
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional cyclic partitioning:
  //
  // [ team 0 | team 1 | team 0 | team 1 | ... ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = dash::math::div_ceil(_num_elem, team_size);
  size_t local_cap  = block_size;
  dash::Pattern<1, dash::ROW_MAJOR> pat_cyclic_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::CYCLIC),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Column order must be irrelevant:
  dash::Pattern<1, dash::COL_MAJOR> pat_cyclic_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::CYCLIC),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_cyclic_row.capacity(), _num_elem);
  EXPECT_EQ(pat_cyclic_row.blocksize(0), 1);
  EXPECT_EQ(pat_cyclic_row.max_elem_per_unit(), local_cap);
  EXPECT_EQ(pat_cyclic_col.capacity(), _num_elem);
  EXPECT_EQ(pat_cyclic_col.blocksize(0), 1);
  EXPECT_EQ(pat_cyclic_col.max_elem_per_unit(), local_cap);
  size_t expected_unit_id;
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int expected_unit_id = x % team_size;
    int expected_offset  = x / team_size;
    int expected_index   = x;
    expected_coords[0]   = x;
    LOG_MESSAGE("x: %d, eu: %d, eo: %d",
      x, expected_unit_id, expected_offset);
    // Row order:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_row.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_row.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_cyclic_row.local_to_global_index(
        expected_unit_id, expected_offset));
    // Column order:
    EXPECT_EQ(
      expected_coords,
      pat_cyclic_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_cyclic_col.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_cyclic_col.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_cyclic_col.local_to_global_index(
        expected_unit_id, expected_offset));
  }
}

TEST_F(PatternTest, Distribute1DimBlockcyclic) {
  DASH_TEST_LOCAL_ONLY();
  // Simple 1-dimensional blocked partitioning:
  //
  // [ team 0 | team 1 | team 0 | team 1 | ... ]
  size_t team_size  = dash::Team::All().size();
  size_t block_size = 23;
  size_t num_blocks = dash::math::div_ceil(_num_elem, block_size);
  size_t local_cap  = block_size *
                        dash::math::div_ceil(num_blocks, team_size);
  dash::Pattern<1, dash::ROW_MAJOR> pat_blockcyclic_row(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKCYCLIC(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  // Column order must be irrelevant:
  dash::Pattern<1, dash::COL_MAJOR> pat_blockcyclic_col(
      dash::SizeSpec<1>(_num_elem),
      dash::DistributionSpec<1>(dash::BLOCKCYCLIC(block_size)),
      dash::TeamSpec<1>(),
      dash::Team::All());
  EXPECT_EQ(pat_blockcyclic_row.capacity(), _num_elem);
  EXPECT_EQ(pat_blockcyclic_row.blocksize(0), block_size);
  EXPECT_EQ(pat_blockcyclic_row.max_elem_per_unit(), local_cap);
  EXPECT_EQ(pat_blockcyclic_col.capacity(), _num_elem);
  EXPECT_EQ(pat_blockcyclic_col.blocksize(0), block_size);
  EXPECT_EQ(pat_blockcyclic_col.max_elem_per_unit(), local_cap);
  size_t expected_unit_id;
  LOG_MESSAGE("num elem: %d, block size: %d, num blocks: %d",
    _num_elem, block_size, num_blocks);
  std::array<long long, 1> expected_coords;
  for (int x = 0; x < _num_elem; ++x) {
    int block_index       = x / block_size;
    int block_base_offset = block_size * (block_index / team_size);
    int expected_unit_id  = block_index % team_size;
    int expected_offset   = (x % block_size) + block_base_offset;
    int expected_index    = x;
    expected_coords[0]    = x;
    LOG_MESSAGE("x: %d, eu: %d, eo: %d, bi: %d bbo: %d",
      x, expected_unit_id, expected_offset, block_index, block_base_offset);
    // Row order:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_row.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_row.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_row.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_blockcyclic_row.local_to_global_index(
        expected_unit_id, expected_offset));
    // Column order:
    EXPECT_EQ(
      expected_coords,
      pat_blockcyclic_col.coords(x));
    EXPECT_EQ(
      expected_unit_id,
      pat_blockcyclic_col.index_to_unit(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_offset,
      pat_blockcyclic_col.index_to_elem(std::array<long long, 1> { x }));
    EXPECT_EQ(
      expected_index,
      pat_blockcyclic_col.local_to_global_index(
        expected_unit_id, expected_offset));
  }
}


TEST_F(PatternTest, Distribute2DimBlockedY) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in second dimension:
  // Row major:
  // [ team 0[0] | team 0[1] | ... | team 0[2] ]
  // [ team 0[3] | team 0[4] | ... | team 0[5] ]
  // [ team 1[0] | team 1[1] | ... | team 1[2] ]
  // [ team 1[3] | team 1[4] | ... | team 1[5] ]
  // [                   ...                   ]
  // Column major:
  // [ team 0[0] | team 0[2] | ... | team 0[4] ]
  // [ team 0[1] | team 0[3] | ... | team 0[5] ]
  // [ team 1[0] | team 1[2] | ... | team 1[4] ]
  // [ team 1[1] | team 1[3] | ... | team 1[5] ]
  // [                   ...                   ]
  int team_size = dash::Team::All().size();
  int extent_x  = 7;
  int extent_y  = 4;
  size_t size   = extent_x * extent_y;
  // Ceil division
  int block_size_x = extent_x;
  int block_size_y = (extent_y % team_size == 0)
                       ? extent_y / team_size
                       : extent_y / team_size + 1;
  int max_per_unit = block_size_x * block_size_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit);
  dash::Pattern<2, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::NONE, dash::BLOCKED),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), size);
  EXPECT_EQ(pat_blocked_row.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  LOG_MESSAGE("block size: x: %d, y: %d",
    block_size_x, block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order =
        expected_index_row_order % max_per_unit;
      int expected_offset_col_order =
        y % block_size_y + x * block_size_y;
      int expected_unit_id = y / block_size_y;
      // Row order:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_row_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_row_order,
        pat_blocked_row.local_to_global_index(
          expected_unit_id, expected_offset_row_order));
      // Col order:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_col_order,
        pat_blocked_col.local_to_global_index(
          expected_unit_id, expected_offset_col_order));
    }
  }
}

TEST_F(PatternTest, Distribute2DimBlockedX) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ team 0[0] | team 1[0] | team 2[0] | ... | team n-1 ]
  // [ team 0[1] | team 1[1] | team 2[1] | ... | team n-1 ]
  // [ team 0[2] | team 1[2] | team 2[2] | ... | team n-1 ]
  // [ team 0[3] | team 1[3] | team 2[3] | ... | team n-1 ]
  // [                       ...                          ]
  int team_size    = dash::Team::All().size();
  int extent_x     = 41;
  int extent_y     = 17;
  size_t size      = extent_x * extent_y;
  // Ceil division
  int block_size_x = dash::math::div_ceil(extent_x, team_size);
  int block_size_y = extent_y;
  int max_per_unit = block_size_x * block_size_y;
  dash::Pattern<2, dash::ROW_MAJOR> pat_blocked_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_blocked_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_blocked_row.capacity(), size);
  EXPECT_EQ(pat_blocked_row.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_blocked_col.capacity(), size);
  EXPECT_EQ(pat_blocked_col.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_blocked_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_blocked_col.blocksize(1), block_size_y);
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_offset_row_order = (x % block_size_x) + y * block_size_x;
      int expected_offset_col_order = expected_index_col_order % 
                                        max_per_unit;
      int expected_unit_id = x / block_size_x;
      // Row order:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_row_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_row.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_blocked_row.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_row_order,
        pat_blocked_row.local_to_global_index(
          expected_unit_id, expected_offset_row_order));
      // Col order:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, ei: %d, bx: %d, by: %d",
        x, y,
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }),
        expected_index_col_order,
        block_size_x, block_size_y);
      EXPECT_EQ(
        expected_unit_id,
        pat_blocked_col.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_blocked_col.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_col_order,
        pat_blocked_col.local_to_global_index(
          expected_unit_id, expected_offset_col_order));
    }
  }
}

TEST_F(PatternTest, Distribute2DimCyclicX) {
  DASH_TEST_LOCAL_ONLY();
  // 2-dimensional, blocked partitioning in first dimension:
  // 
  // [ team 0[0] | team 1[0] | team 0[1] | team 1[1] | ... ]
  // [ team 0[2] | team 1[2] | team 0[3] | team 1[3] | ... ]
  // [ team 0[4] | team 1[4] | team 0[5] | team 1[5] | ... ]
  // [ team 0[6] | team 1[6] | team 0[7] | team 1[7] | ... ]
  // [                        ...                          ]
  int team_size      = dash::Team::All().size();
  // Choose 'inconvenient' extents:
  // int extent_x       = team_size + 7;
  // int extent_y       = 23;
  int extent_x       = 8;
  int extent_y       = 4;
  size_t size        = extent_x * extent_y;
  int block_size_x   = 1;
  int max_per_unit_x = dash::math::div_ceil(extent_x, team_size);
  int block_size_y   = extent_y;
  int max_per_unit   = max_per_unit_x * block_size_y;
  LOG_MESSAGE("ex: %d, ey: %d, bsx: %d, bsy: %d, mpx: %d, mpu: %d",
      extent_x, extent_y,
      block_size_x, block_size_y,
      max_per_unit_x, max_per_unit);
  dash::Pattern<2, dash::ROW_MAJOR> pat_cyclic_row(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  dash::Pattern<2, dash::COL_MAJOR> pat_cyclic_col(
      dash::SizeSpec<2>(extent_x, extent_y),
      dash::DistributionSpec<2>(dash::CYCLIC, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()),
      dash::Team::All());
  EXPECT_EQ(pat_cyclic_row.capacity(), size);
  EXPECT_EQ(pat_cyclic_row.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_cyclic_row.blocksize(0), block_size_x);
  EXPECT_EQ(pat_cyclic_row.blocksize(1), block_size_y);
  EXPECT_EQ(pat_cyclic_col.capacity(), size);
  EXPECT_EQ(pat_cyclic_col.max_elem_per_unit(), max_per_unit);
  EXPECT_EQ(pat_cyclic_col.blocksize(0), block_size_x);
  EXPECT_EQ(pat_cyclic_col.blocksize(1), block_size_y);
  // offset to add for every y-coordinate:
  int add_offset_per_y    = extent_x / team_size;
  // number of overflow blocks, e.g. 7 elements, 3 teams -> 1
  int num_overflow_blocks = extent_x % team_size;
  for (int x = 0; x < extent_x; ++x) {
    for (int y = 0; y < extent_y; ++y) {
      int expected_index_row_order  = (y * extent_x) + x;
      int expected_index_col_order  = (x * extent_y) + y;
      int expected_unit_id = x % team_size;
      int expected_offset_row_order =
        x / team_size + (y * (add_offset_per_y + num_overflow_blocks - x));
      int expected_offset_col_order =
        expected_offset_row_order;
      // Row order:
      LOG_MESSAGE("R x: %d, y: %d, eo: %d, ao: %d, of: %d",
        x, y,
        expected_offset_row_order,
        pat_cyclic_row.index_to_elem(std::array<long long, 2> { x, y }),
        num_overflow_blocks);
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_row.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_row_order,
        pat_cyclic_row.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_row_order,
        pat_cyclic_row.local_to_global_index(
          expected_unit_id, expected_offset_row_order));
      // Col order:
      LOG_MESSAGE("C x: %d, y: %d, eo: %d, ao: %d, of: %d",
        x, y,
        expected_offset_col_order,
        pat_cyclic_col.index_to_elem(std::array<long long, 2> { x, y }),
        num_overflow_blocks);
      EXPECT_EQ(
        expected_unit_id,
        pat_cyclic_col.index_to_unit(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_offset_col_order,
        pat_cyclic_col.index_to_elem(std::array<long long, 2> { x, y }));
      EXPECT_EQ(
        expected_index_col_order,
        pat_cyclic_col.local_to_global_index(
          expected_unit_id, expected_offset_col_order));
    }
  }
}

