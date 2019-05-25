
#include <gtest/gtest.h>

#include "../TestBase.h"
#include "ReduceTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Reduce.h>
#include <dash/algorithm/Fill.h>

#include <array>


TEST_F(ReduceTest, SimpleStart) {
  const size_t num_elem_local = 100;
  size_t num_elem_total       = _dash_size * num_elem_local;
  auto value = 2, start = 10;

  dash::Array<int> target(num_elem_total, dash::BLOCKED);

  dash::fill(target.begin(), target.end(), value);

  dash::barrier();

  int result = dash::reduce(target.begin(),
        target.end(),
        start); //start value

  ASSERT_EQ_U(num_elem_total * value + start, result);

  result = dash::reduce(target.begin(), target.end(), 0);
  ASSERT_EQ_U(num_elem_total * value, result);

  result = dash::reduce(target.lbegin(), target.lend(), 0);
  ASSERT_EQ_U(num_elem_total * value, result);
}


TEST_F(ReduceTest, OpMult) {
  const size_t num_elem_local = 1;
  using value_t = uint64_t;
  size_t num_elem_total       = std::max(static_cast<size_t>(32), dash::size());
  value_t value = 2, start = 10;

  dash::Array<uint64_t> target(num_elem_total, dash::BLOCKED);

  dash::fill(target.begin(), target.end(), value);

  dash::barrier();

  auto result = dash::reduce(target.begin(),
                             target.end(),
                             start, //start value
                             dash::multiply<value_t>());

  ASSERT_EQ_U((1ULL<<num_elem_total) * start, result);
}


TEST_F(ReduceTest, SimpleStruct) {
  struct value_struct {
    int x{0}, y{0};
    value_struct() = default;
    value_struct(int _x, int _y)
      : x(_x)
      , y(_y)
    { }
    value_struct operator+(const value_struct& rhs) const {
      value_struct result(x, y);
      result.x += rhs.x;
      result.y += rhs.y;
      return result;
    }
    value_struct& operator+=(const value_struct& rhs) {
      this->x += rhs.x;
      this->y += rhs.y;
      return *this;
    }
  };

  const size_t num_elem_local = 100;
  size_t num_elem_total       = _dash_size * num_elem_local;
  constexpr int x = 1, y = 2;
  value_struct value(x, y);

  dash::Array<value_struct> target(num_elem_total, dash::BLOCKED);

  dash::fill(target.begin(), target.end(), value);

  dash::barrier();

  // full-range reduce
  auto result = dash::reduce(target.begin(), target.end(),
                             value_struct(10, 20));

  ASSERT_EQ_U(num_elem_total * x + 10, result.x);
  ASSERT_EQ_U(num_elem_total * y + 20, result.y);

  // half-range reduce
  result = dash::reduce(target.begin(), target.begin() + num_elem_total/2,
                        value_struct(10, 20));

  ASSERT_EQ_U(num_elem_total/2 * x + 10, result.x);
  ASSERT_EQ_U(num_elem_total/2 * y + 20, result.y);
}


TEST_F(ReduceTest, StringConcatOperaton) {
  const size_t num_elem_local = 100;
  size_t num_elem_total       = _dash_size * num_elem_local;
  auto value = 2;

  // Create a vector
  dash::Array<int> target(4);
  target[0] = 1;
  target[1] = 2;
  target[2] = 3;
  target[3] = 4;

  dash::barrier();

  std::string result = std::accumulate(
				       std::next(target.begin()),
				       target.end(),
				       std::to_string(target[0]), // start element
                                    [](std::string x1, int x2) {
                                        return x1 + '-' + std::to_string(x2);
                                    });

  if(dash::myid() == 0) {
    ASSERT_STREQ("1-2-3-4", result.c_str());
  }
}


TEST_F(ReduceTest, LocalPredefined) {
  int value = 2;

  // perform a reduction similar to MPI_Allreduce
  auto result = dash::reduce(&value, std::next(&value), 1, dash::plus<int>(), true);


  ASSERT_EQ_U(dash::size()*value + 1, result);
}

TEST_F(ReduceTest, LocalStdIterator) {
  std::list<int> list;
  list.push_back(1*dash::myid());
  list.push_back(2*dash::myid());
  list.push_back(3*dash::myid());

  auto result = dash::reduce(list.begin(), list.end(),
                             1, dash::plus<int>(), true);

  ASSERT_EQ_U(((dash::size()-1)*(dash::size())/2) * (1 + 2 + 3)  + 1, result);
}
