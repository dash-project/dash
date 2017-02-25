
#include "RangeTest.h"

#include <gtest/gtest.h>

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <array>


TEST_F(RangeTest, RangeTraits)
{
  dash::Array<int> array(dash::size() * 10);
  auto v_sub  = dash::sub(0, 10, array);
  auto i_sub  = dash::index(v_sub);
  auto v_ssub = dash::sub(0, 5, (dash::sub(0, 10, array)));
  auto v_loc  = dash::local(array);

  static_assert(dash::is_range<
                   dash::Array<int>
                >::value == true,
                "dash::is_range<dash::Array>::value not matched");

  static_assert(dash::is_range<
                   typename dash::Array<int>::local_type
                >::value == true,
                "dash::is_range<dash::Array::local_type>::value not matched");

  static_assert(dash::is_range<
                   typename dash::Array<int>::iterator
                >::value == false,
                "dash::is_range<dash::Array<...>>::value not matched");
  static_assert(dash::is_range<decltype(array)>::value == true,
                "range type traits for dash::Array not matched");
  static_assert(dash::is_range<decltype(v_loc)>::value == true,
                "range type traits for local(dash::Array) not matched");
  static_assert(dash::is_range<decltype(v_sub)>::value == true,
                "range type traits for sub(dash::Array) not matched");
  static_assert(dash::is_range<decltype(v_ssub)>::value == true,
                "range type traits for sub(sub(dash::Array)) not matched");
  static_assert(dash::is_range<decltype(i_sub)>::value == true,
                "range type traits for index(sub(dash::Array)) not matched");

  static_assert(
      dash::is_range<decltype(array)>::value == true,
      "dash::is_range<dash::Array<...>>::value not matched");

  auto l_range = dash::make_range(array.local.begin(),
                                  array.local.end());
  static_assert(
      dash::is_range<decltype(l_range)>::value == true,
      "dash::is_range<dash::make_range(...)>::value not matched");
}

