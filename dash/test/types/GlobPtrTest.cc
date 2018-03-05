
#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "GlobPtrTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Copy.h>


TEST_F(GlobPtrTest, CastAssignment)
{
  auto igptr = dash::memalloc<int>(1);
  auto cgptr = dash::memalloc<char>(1);
  dash::memfree(cgptr);

  igptr[0] = 255;
  ASSERT_EQ_U(255, igptr[0]);

  // should not work!
  //decltype(igptr) igptr2 = dgptr;

  // should do!
  cgptr = static_cast<decltype(cgptr)>(igptr);

  cgptr[0] = 0;
  cgptr[1] = 0;
  cgptr[2] = 0;
  cgptr[3] = 0;

  ASSERT_EQ_U(0, igptr[0]);

  dash::memfree(igptr);

}
