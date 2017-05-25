
#include "RangeTest.h"

#include <gtest/gtest.h>

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <dash/internal/StreamConversion.h>

#include <array>
#include <iterator>



TEST_F(RangeTest, RangeTraits)
{
  dash::Array<int> array(dash::size() * 10);
  auto v_sub  = dash::sub(0, 10, array);
  auto i_sub  = dash::index(v_sub);
  auto v_ssub = dash::sub(0, 5, (dash::sub(0, 10, array)));
  auto v_loc  = dash::local(array);
  auto i_loc  = dash::index(dash::local(array));
// auto v_gloc = dash::global(dash::local(array));
// auto i_glo  = dash::global(dash::index(dash::local(array)));
  auto v_bsub = *dash::begin(dash::blocks(v_sub));

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
                "range type trait for dash::Array not matched");
  static_assert(dash::is_range<decltype(v_loc)>::value == true,
                "range type trait for local(dash::Array) not matched");
  static_assert(dash::is_range<decltype(v_sub)>::value == true,
                "range type trait for sub(dash::Array) not matched");
  static_assert(dash::is_range<decltype(v_ssub)>::value == true,
                "range type trait for sub(sub(dash::Array)) not matched");
  static_assert(dash::is_range<decltype(v_bsub)>::value == true,
                "range type trait for begin(blocks(sub(dash::Array))) "
                "not matched");
  static_assert(dash::is_range<decltype(i_sub)>::value == true,
                "range type trait for index(sub(dash::Array)) not matched");
  static_assert(dash::is_range<decltype(i_loc)>::value == true,
                "range type trait for index(local(dash::Array)) not matched");

  // Index set iterators implement random access iterator concept:
  //
  static_assert(std::is_same<
                  typename std::iterator_traits<
                             decltype(i_sub.begin())
                           >::iterator_category,
                  std::random_access_iterator_tag
                >::value == true,
                "iterator trait iterator_category of "
                "index(local(dash::Array))::iterator not matched");
  static_assert(std::is_same<
                  typename std::iterator_traits<
                             decltype(i_loc.begin())
                           >::iterator_category,
                  std::random_access_iterator_tag
                >::value == true,
                "iterator trait iterator_category of "
                "index(local(dash::Array))::iterator not matched");
//static_assert(std::is_same<
//                typename std::iterator_traits<
//                           decltype(i_glo.begin())
//                         >::iterator_category,
//                std::random_access_iterator_tag
//              >::value == true,
//              "iterator trait iterator_category of "
//              "global(index(local(dash::Array)))::iterator not matched");

  static_assert(
      dash::is_range<decltype(array)>::value == true,
      "dash::is_range<dash::Array<...>>::value not matched");

// auto l_range = dash::make_range(array.local.begin(),
//                                 array.local.end());
// static_assert(
//     dash::is_range<decltype(l_range)>::value == true,
//     "dash::is_range<dash::make_range(...)>::value not matched");
// static_assert(
//     dash::is_view<decltype(l_range)>::value == true,
//     "dash::is_view<dash::make_range(...)>::value not matched");
}

