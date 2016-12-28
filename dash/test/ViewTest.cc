
#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "ViewTest.h"

#include <array>

#if 0
// TODO tf: fix interface of dash::sub
TEST_F(ViewTest, ArrayBlockedPatternView)
{
  int block_size = 37;

  dash::Array<int> a(block_size * dash::size());

  // View to global index range of local block:
  auto block_gview = dash::sub(block_size * dash::myid(),
                               block_size * dash::myid() + 1,
                               a);

  EXPECT_EQ(block_size, block_gview.size());

  auto view_begin_gidx = dash::index(dash::begin(block_gview));
  auto view_end_gidx   = dash::index(dash::end(block_gview));
}
#endif

