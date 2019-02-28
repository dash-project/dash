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

  array_t arr(dash::size());

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

TEST_F(GlobRefTest, ConstCorrectness)
{
  dash::Array<int>     dArray{100};
  std::array<int, 100> stdArray{};

  // OK
  int&               ref  = stdArray[0];
  dash::GlobRef<int> gref = dArray[0];

  // OK as well
  int const&               cref  = ref;
  dash::GlobRef<const int> cgref = gref;

  // NOT OK, because...
  // We must not assign a non-const to const -> Compilation error
  // int&               ref2  = cref;
  // dash::GlobRef<int> gref2 = cgref;
}

TEST_F(GlobRefTest, InheritanceTest)
{
  dash::Array<Child>     dArray{100};
  std::array<Child, 100> stdArray{};

  Child child;
  child.x = 12;
  child.y = 34;

  dash::fill(dArray.begin(), dArray.end(), child);
  std::fill(stdArray.begin(), stdArray.end(), child);

  dArray.barrier();

  auto lpos = dArray.pattern().local(10);

  child.x = 56;
  child.y = 123;
  if (lpos.unit == dash::team_unit_t{dash::myid()}) {
    dArray.local[lpos.index] = child;
  }

  stdArray[lpos.index] = child;

  dArray.barrier();

  Child&               asChild_array  = stdArray[10];
  dash::GlobRef<Child> asChild_darray = dArray[10];

  /*
   * Here we explicitly cast it as Parent. In consequence, we read only 4
   * bytes (i.e., sizeof Parent), instead of 8.
   */
  Parent&               asParent_array  = stdArray[10];
  dash::GlobRef<Parent> asParent_darray = dArray[10];

  // static downcast is allowed with non-virtual base classes:
  // see https://en.cppreference.com/w/cpp/language/static_cast, point 2
  Child& asChild_array2 = static_cast<Child&>(asParent_array);
  //TODO rko: Still to implement -> add in traits
  //dash::GlobRef<Child> asChild_darray2 = static_cast<dash::GlobRef<Child>>(asParent_darray);

  EXPECT_EQ_U(asParent_array.x, 56);
  EXPECT_EQ_U(asChild_array.y, 123);

  EXPECT_EQ_U(static_cast<Child>(asChild_darray).y, 123);
  //Look into the logs and grep for dart_get_blocking to see that we really
  //get only 4 bytes instead of 8.
  EXPECT_EQ_U(static_cast<Parent>(asParent_darray).x, 56);
}
