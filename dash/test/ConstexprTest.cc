
#include "ConstexprTest.h"

#include <gtest/gtest.h>

#include <dash/util/FunctionalExpr.h>
#include <dash/util/ArrayExpr.h>


TEST_F(ConstexprTest, Accumulate)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr int acc = dash::ce::accumulate(
                        arr, 0, 9,
                        100, dash::ce::plus<int>);
  
  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.Accumulate", acc);
  }

  EXPECT_EQ_U(136, acc);
}

TEST_F(ConstexprTest, SplitArray)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr int split_ix = 4;

  constexpr dash::ce::split_array<int, 9, split_ix> arr_split(arr);
  
  constexpr std::array<int, 4> exp_l = { 0, 1, 2, 3 };
  constexpr std::array<int, 5> exp_r = { 4, 5, 6, 7, 8 };
  
  auto & arr_l = arr_split.left();
  auto & arr_r = arr_split.right();

  if (dash::myid() == 0) {
    for (auto l_elem : arr_l) {
      DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", l_elem);
    }
    for (auto r_elem : arr_r) {
      DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", r_elem);
    }
  }

  EXPECT_EQ_U(exp_l, arr_l);
  EXPECT_EQ_U(exp_r, arr_r);
}

