
#include "ConstexprTest.h"

#include <gtest/gtest.h>

#include <dash/util/FunctionalExpr.h>
#include <dash/util/ArrayExpr.h>


TEST_F(ConstexprTest, Accumulate)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  {
    constexpr int acc = dash::ce::accumulate(
                          arr, 0, 9,
                          100, dash::ce::plus<int>);

    if (dash::myid() == 0) {
      DASH_LOG_DEBUG_VAR("ConstexprTest.Accumulate", acc);
      EXPECT_EQ_U(136, acc);
    }
  }
  {
    constexpr int acc = dash::ce::accumulate(
                          arr, 2, 8,
                          100, dash::ce::plus<int>);

    if (dash::myid() == 0) {
      DASH_LOG_DEBUG_VAR("ConstexprTest.Accumulate", acc);
      EXPECT_EQ_U(127, acc);
    }
  }
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

  constexpr auto takedrop = dash::ce::take<5>(
                              dash::ce::drop<3>(arr));

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.Append", takedrop);
  }
  EXPECT_EQ_U(exp, takedrop);

  constexpr std::array<int, 0> empty {{ }};
  constexpr auto drop_all = dash::ce::drop<9>(arr);

  EXPECT_EQ_U(empty, drop_all);
}

TEST_F(ConstexprTest, HeadTail)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr auto arr_head = dash::ce::head(arr);
  constexpr auto arr_tail = dash::ce::tail(arr);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.HeadTail", arr_head);
    DASH_LOG_DEBUG_VAR("ConstexprTest.HeadTail", arr_tail);
  }

  EXPECT_EQ_U(1,              arr_head.size());
  EXPECT_EQ_U(arr.size() - 1, arr_tail.size());

  constexpr auto arr_join = dash::ce::append(arr_head, arr_tail);

  EXPECT_EQ_U(arr, arr_join);
}

TEST_F(ConstexprTest, Reverse)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr auto arr_rev = dash::ce::reverse(arr);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.Reverse", arr_rev);
  }

  constexpr std::array<int, 9> exp_rev = { 8, 7, 6, 5, 4, 3, 2, 1, 0 };
  EXPECT_EQ_U(exp_rev, arr_rev);
}

TEST_F(ConstexprTest, ReplaceNth)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr auto arr_rep = dash::ce::replace_nth<3>(123, arr);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.ReplaceNth", arr_rep);
  }
}

TEST_F(ConstexprTest, Split)
{
  constexpr std::array<int, 9> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

  constexpr int nleft  = 4;
  constexpr int nright = 5;

  constexpr dash::ce::split<int, nleft, nright> arr_split(arr);

  constexpr std::array<int, nleft>  exp_l = { 0, 1, 2, 3 };
  constexpr std::array<int, nright> exp_r = { 4, 5, 6, 7, 8 };

  constexpr auto arr_l = arr_split.left();
  constexpr auto arr_r = arr_split.right();

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", arr_l);
    DASH_LOG_DEBUG_VAR("ConstexprTest.SplitArray", arr_r);
  }

  EXPECT_EQ_U(exp_l, arr_l);
  EXPECT_EQ_U(exp_r, arr_r);
}

