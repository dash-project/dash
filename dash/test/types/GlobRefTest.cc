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
  int &              ref  = stdArray[0];
  dash::GlobRef<int> gref = dArray[0];

  // OK as well
  int const &cref = ref;
  //
  //
  dash::GlobRef<const int> cgref{gref};
  //
  //

  static_assert(
      dash::detail::is_implicitly_convertible<int, const int>::value, "");
  static_assert(
      !dash::detail::is_implicitly_convertible<int const, int>::value, "");

  // NOT OK, because...
  // We must not assign a non-const to const -> Compilation error
  // int&               ref2  = cref;
  // dash::GlobRef<int> gref2 = cgref;
}

TEST_F(GlobRefTest, InheritanceConversionTest)
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

  Child &              child_stdArray = stdArray[10];
  dash::GlobRef<Child> child_dArray   = dArray[10];

  /*
   * Here we explicitly cast it as Parent. In consequence, we read only 4
   * bytes (i.e., sizeof Parent), instead of 8.
   */
  Parent &              upcastParent_stdArray = stdArray[10];
  dash::GlobRef<Parent> upcastParent_dArray   = dArray[10];

  auto const  val  = child;
  auto const &cref = val;

  // Why can we assign a const val to
  auto &r_auto = val;
  // The reason this works is the following
  static_assert(std::is_same<Child const &, decltype(r_auto)>::value, "");
  // => auto type deduction includes the const modifer as well.
  // However, the following is malformed.
  // Child & ref = cref;

  // But this does not work anymore...
  // Child& r_Child = val;

  Parent const &r_upcast   = r_auto;
  Child const & r_downcast = static_cast<Child const &>(r_upcast);

  // static downcast is allowed with non-virtual base classes:
  // see https://en.cppreference.com/w/cpp/language/static_cast, point 2
  Child &downcastChild_stdArray = static_cast<Child &>(upcastParent_stdArray);
  dash::GlobRef<Child> downcastChild_dArray =
      static_cast<dash::GlobRef<Child>>(upcastParent_dArray);

  EXPECT_EQ_U(child_stdArray.y, 123);
  EXPECT_EQ_U(static_cast<Child>(child_dArray).y, 123);

  EXPECT_EQ_U(upcastParent_stdArray.x, 56);
  EXPECT_EQ_U(downcastChild_stdArray.y, 123);

  // Look into the logs and grep for dart_get_blocking to see that we really
  // get only 4 bytes instead of 8.
  EXPECT_EQ_U(static_cast<Parent>(upcastParent_dArray).x, 56);
  EXPECT_EQ_U(static_cast<Child>(downcastChild_dArray).y, 123);
}

template <class T>
using dash_ref = dash::GlobRef<T>;

TEST_F(GlobRefTest, ConversionRules)
{
  static_assert(
      std::is_convertible<dash_ref<int>, dash_ref<const int>>::value, "1.1");
  static_assert(std::is_convertible<int &, const int &>::value, "1.2");

  static_assert(
      !std::is_convertible<dash_ref<const int>, dash_ref<int>>::value, "2.1");
  static_assert(!std::is_convertible<const int &, int &>::value, "2.2");

  static_assert(
      !std::is_convertible<dash_ref<int>, dash_ref<double>>::value, "3.1");
  static_assert(!std::is_convertible<int &, double &>::value, "3.2");

  static_assert(
      std::is_convertible<dash_ref<Child>, dash_ref<Parent>>::value, "4.1");
  static_assert(std::is_convertible<Child &, Parent &>::value, "4.2");

  static_assert(
      std::is_convertible<dash_ref<Child>, dash_ref<const Parent>>::value,
      "5.1");
  static_assert(std::is_convertible<Child &, const Parent &>::value, "5.2");

  static_assert(!std::is_convertible<const Child &, Parent &>::value, "6.1");

  static_assert(
      !std::is_convertible<dash_ref<const Child>, dash_ref<Parent>>::value,
      "7.1");
  static_assert(!std::is_convertible<const Child &, Parent &>::value, "7.2");
  static_assert(
      !dash::detail::is_explicitly_convertible<const Child &, Parent &>::
          value,
      "7.3");
  static_assert(
      !dash::detail::is_explicitly_convertible<
          dash_ref<const Child>,
          dash_ref<Parent>>::value,
      "7.4");

  static_assert(!std::is_convertible<Parent &, Child const &>::value, "8.1");
  static_assert(
      !std::is_convertible<dash_ref<Parent>, dash_ref<const Child>>::value,
      "8.2");
  static_assert(
      dash::detail::is_explicitly_convertible<Parent &, Child const &>::value,
      "8.3");
  static_assert(
      dash::detail::is_explicitly_convertible<
          dash_ref<Parent>,
          dash_ref<const Child>>::value,
      "8.4");

  static_assert(!std::is_convertible<Parent &, Child &>::value, "9.1");
  static_assert(
      !std::is_convertible<dash_ref<Parent>, dash_ref<Child>>::value, "9.2");
  static_assert(
      dash::detail::is_explicitly_convertible<Parent &, Child &>::value,
      "9.3");
  static_assert(
      dash::detail::
          is_explicitly_convertible<dash_ref<Parent>, dash_ref<Child>>::value,
      "9.4");
}
