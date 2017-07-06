
#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "GlobRefTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Copy.h>


TEST_F(GlobRefTest, ArithmeticOps)
{
  using value_t = int;
  dash::Array<value_t> arr(dash::size());
  int neighbor = (dash::myid() + 1) % dash::size();
  dash::GlobRef<value_t> gref = arr[neighbor];

  // assignment
  gref = 0;
  ASSERT_EQ_U(gref, 0);

  // prefix increment
  ++gref;
  ASSERT_EQ_U(gref, 1);

  ++(++gref);
  ASSERT_EQ_U(gref, 3);

  // prefix decrement
  --(--gref);
  ASSERT_EQ_U(gref, 1);

  // unary operations
  gref *= 2;
  ASSERT_EQ_U(gref, 2);

  gref /= 2;
  ASSERT_EQ_U(gref, 1);

  gref += 1;
  ASSERT_EQ_U(gref, 2);

  gref -= 1;
  ASSERT_EQ_U(gref, 1);

  // postfix increment
  value_t prev = gref++;
  ASSERT_EQ_U(prev, 1);
  ASSERT_EQ_U(gref, 2);


  // postfix increment
  prev = gref--;
  ASSERT_EQ_U(prev, 2);
  ASSERT_EQ_U(gref, 1);

}
