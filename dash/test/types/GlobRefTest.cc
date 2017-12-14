
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
  ASSERT_EQ_U(++gref, 1);
  ASSERT_EQ_U(gref, 1);

  ASSERT_EQ_U(++(++gref), 3);
  ASSERT_EQ_U(gref, 3);

  // postfix increment
  ASSERT_EQ_U(gref++, 3);
  ASSERT_EQ_U(gref, 4);

  // postfix decrement
  ASSERT_EQ_U(gref--, 4);
  ASSERT_EQ_U(gref, 3);

  // prefix decrement
  ASSERT_EQ_U(--(--gref), 1);
  ASSERT_EQ_U(gref, 1);

  // unary operations
  ASSERT_EQ_U(gref *= 2, 2);
  ASSERT_EQ_U(gref, 2);

  ASSERT_EQ_U(gref /= 2, 1);
  ASSERT_EQ_U(gref, 1);

  ASSERT_EQ_U(gref += 1, 2);
  ASSERT_EQ_U(gref, 2);

  ASSERT_EQ_U(gref -= 1, 1);
  ASSERT_EQ_U(gref, 1);
}
