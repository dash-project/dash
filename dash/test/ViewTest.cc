
#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "ViewTest.h"

#include <array>


TEST_F(ViewTest, ArrayBlockedPatternView)
{
  int block_size       = 37;
  int block_begin_gidx = block_size * dash::myid();
  int block_end_gidx   = block_size * (dash::myid() + 1);

  dash::Array<int> a(block_size * dash::size());

  // View to global index range of local block:
  auto block_gview = dash::sub(block_begin_gidx,
                               block_end_gidx,
                               a);

  EXPECT_EQ(block_size, block_gview.size());

  auto view_begin_gidx = dash::index(dash::begin(block_gview));
  auto view_end_gidx   = dash::index(dash::end(block_gview));

  EXPECT_EQ(block_begin_gidx, view_begin_gidx);
  EXPECT_EQ(block_end_gidx,   view_end_gidx);
}

