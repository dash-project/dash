
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

TEST_F(ConstexprTest, Append)
{
  constexpr std::array<int, 5> arr_l   = { 0, 1, 2, 3, 4 };
  constexpr std::array<int, 4> arr_r   = { 5, 6, 7, 8 };

  constexpr auto arr_app = dash::ce::append(arr_l, arr_r);

  constexpr std::array<int, 9> exp_app = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.Append", arr_app);
  }
  EXPECT_EQ_U(exp_app, arr_app);

  constexpr auto arr_add = dash::ce::append(arr_l, 23);

  constexpr std::array<int, 6> exp_add = { 0, 1, 2, 3, 4, 23 };
  EXPECT_EQ_U(exp_add, arr_add);
}

TEST_F(ConstexprTest, TakeDrop)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
  constexpr std::array<int, 5> exp = { 3, 4, 5, 6, 7 };

  constexpr auto takedrop = dash::ce::take<int, 6, 5>(
                              dash::ce::drop<int, 9, 3>(arr));

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.Append", takedrop);
  }
  EXPECT_EQ_U(exp, takedrop);
}

TEST_F(ConstexprTest, Split)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr int split_ix = 4;

  constexpr dash::ce::split<int, 9, split_ix> arr_split(arr);
  
  constexpr std::array<int, 4> exp_l = { 0, 1, 2, 3 };
  constexpr std::array<int, 5> exp_r = { 4, 5, 6, 7, 8 };
  
  constexpr auto arr_l = arr_split.left();
  constexpr auto arr_r = arr_split.right();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", arr_l);
    DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", arr_r);
  }

  EXPECT_EQ_U(exp_l, arr_l);
  EXPECT_EQ_U(exp_r, arr_r);
}

