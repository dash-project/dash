
#include <gtest/gtest.h>

#include "../TestBase.h"
#include "BcastTest.h"

#include <dash/Array.h>
#include <dash/Shared.h>
#include <dash/algorithm/Bcast.h>
#include <dash/algorithm/Fill.h>

#include <array>


TEST_F(BcastTest, Shared) {
  constexpr int xval = 1001;
  constexpr int yval = 1002;
  struct point_t { int x, y; };
  using value_t = point_t;
  dash::Shared<point_t> shared({xval, yval});

  auto res = dash::broadcast(shared);

  ASSERT_EQ_U(res.x, xval);
  ASSERT_EQ_U(res.y, yval);
}

TEST_F(BcastTest, Vector) {
  constexpr int num_elem_per_unit = 10;
  struct point_t {
    int x, y;

    point_t&
    operator++(){
      ++x;
      ++y;
      return *this;
    }
  };
  using value_t = point_t;

  std::vector<point_t> vec(num_elem_per_unit);

  std::iota(vec.begin(), vec.end(), point_t{dash::myid(), 1000+dash::myid()});

  dash::broadcast(vec.begin(), vec.end(),
              dash::team_unit_t{static_cast<int>(dash::size())-1});

  for (int i = 0; i < num_elem_per_unit; ++i) {
    ASSERT_EQ_U(vec[i].x, (dash::size() - 1 + i));
    ASSERT_EQ_U(vec[i].y, (1000 + dash::size() - 1 + i));
  }

}

TEST_F(BcastTest, List) {
  constexpr int num_elem_per_unit = 10;
  struct point_t {
    int x, y;

    point_t&
    operator++(){
      ++x;
      ++y;
      return *this;
    }
  };
  using value_t = point_t;

  std::list<point_t> list(num_elem_per_unit);

  std::iota(list.begin(), list.end(), point_t{dash::myid(), 1000+dash::myid()});

  dash::broadcast(list.begin(), list.end(),
              dash::team_unit_t{static_cast<int>(dash::size())-1});

  int cnt = 0;
  for (auto& elem : list) {
    ASSERT_EQ_U(elem.x, (dash::size() - 1 + cnt));
    ASSERT_EQ_U(elem.y, (1000 + dash::size() - 1 + cnt));
    cnt++;
  }

}
