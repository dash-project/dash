
#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "ViewTest.h"

#include <array>

TEST_F(ViewTest, ArrayBlockedPatternGlobalView)
{
  int block_size       = 37;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size);

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  // Origin of block view is array:
  auto & block_origin = dash::origin(block_gview);

  EXPECT_EQ(array_size, block_origin.size());
  EXPECT_EQ(a.begin(),  dash::begin(block_origin));
  EXPECT_EQ(a.end(),    dash::end(block_origin));

  auto view_begin_gidx = dash::index(dash::begin(block_gview));
  auto view_end_gidx   = dash::index(dash::end(block_gview));

  EXPECT_EQ(block_begin_gidx, view_begin_gidx);
  EXPECT_EQ(block_end_gidx,   view_end_gidx);
}

TEST_F(ViewTest, ArrayBlockedPatternChainedGlobalView)
{
  int block_size       = 37;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size);

  // View to global index range of local block:
  auto block_gview_outer = dash::sub(block_begin_gidx,
                                     block_end_gidx,
                                     a);
  // Sub-range in block from block index 10 to -10:
  auto block_gview_inner = dash::sub(10,
                                     block_size-10,
                                     block_gview_outer);
  EXPECT_EQ(block_size - 10 - 10,  block_gview_inner.size());
  EXPECT_EQ(block_begin_gidx + 10,
            dash::index(dash::begin(block_gview_inner)));
  EXPECT_EQ(block_begin_gidx + block_size - 10,
            dash::index(dash::end(block_gview_inner)));

  // Origin of inner view is outer view:
  auto & block_gview_inner_origin = dash::origin(block_gview_inner);
  EXPECT_EQ(block_gview_outer, block_gview_inner_origin);

  // Origin of outer view is array:
  auto & block_gview_outer_origin = dash::origin(block_gview_outer);
  EXPECT_EQ(a.begin(),  dash::begin(block_gview_outer_origin));
  EXPECT_EQ(a.end(),    dash::end(block_gview_outer_origin));
}

TEST_F(ViewTest, ArrayBlockCyclicPatternGlobalView)
{
  int block_size       = 37;
  int blocks_per_unit  = 3;
  int array_size       = dash::size() * block_size * blocks_per_unit;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size, dash::BLOCKCYCLIC(block_size));

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  // Origin of block view is array:
  auto & block_origin = dash::origin(block_gview);
  EXPECT_EQ(a.begin(),  dash::begin(block_origin));
  EXPECT_EQ(a.end(),    dash::end(block_origin));
}

TEST_F(ViewTest, ArrayBlockedPatternLocalView)
{
  int block_size       = 37;
  int array_size       = dash::size() * block_size;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(array_size);

  // Use case:
  //
  //   local(sub(i0,ik, array)
  //

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);
  EXPECT_EQ(block_size, block_gview.size());

  auto block_lview = dash::local(block_gview);

  // Origin of local block view is local array index space:
  auto & block_lview_origin = dash::origin(block_lview);
  auto & block_gview_origin = dash::origin(block_lview_origin);
}

