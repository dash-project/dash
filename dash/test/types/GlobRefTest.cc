#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "GlobRefTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>

TEST_F(GlobRefTest, ArithmeticOps)
{
  using value_t = int;
  using array_t = dash::Array<value_t>;
  array_t                arr(dash::size());
  int                    neighbor = (dash::myid() + 1) % dash::size();
  dash::GlobRef<value_t> gref     = arr[neighbor];

  auto address_of_ref = dash::addressof<typename array_t::memory_type>(gref);

  ASSERT_EQ_U(
      static_cast<typename array_t::pointer>(arr.begin()) + neighbor,
      address_of_ref);

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

struct Parent {
  int x;
};

struct Child : public Parent {
  int y;
};

namespace dash {
template <>
struct is_container_compatible<Child> : public std::true_type {
};
}  // namespace dash

TEST_F(GlobRefTest, InheritanceTest)
{
  dash::Array<Child> array{100};

  Child child;
  child.x = 12;
  child.y = 34;

  dash::fill(array.begin(), array.end(), child);

  array.barrier();

  auto lpos = array.pattern().local(10);

  if (lpos.unit == static_cast<dash::team_unit_t>(dash::myid())) {
    child.x                 = 56;
    child.y                 = 123;
    array.local[lpos.index] = child;
  }

  array.barrier();

  auto asChild = array[10].get();

  /*
   * Here we explicitly cast it as Parent. In consequence, we read only 4
   * bytes (i.e., sizeof Parent), instead of 8.
   */
  auto asParent = static_cast<dash::GlobRef<Parent>>(array[10]).get();

  EXPECT_EQ_U(asParent.x, 56);
  EXPECT_EQ_U(asChild.y, 123);
}
