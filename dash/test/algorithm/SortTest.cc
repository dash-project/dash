#include "SortTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Sort.h>

#include <algorithm>
#include <random>

template <typename GlobIter>
void perform_test(GlobIter begin, GlobIter end);

TEST_F(SortTest, ArrayBlockedFullRange)
{
  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  static std::uniform_int_distribution<Element_t> distribution(-1000, 1000);
  static std::random_device                       rd;
  static std::mt19937 generator(rd() + dash::myid());

  size_t num_local_elem = 100;

  LOG_MESSAGE("SortTest.ArrayBlockedFullRange: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  std::generate(
      array.lbegin(), array.lend(), []() { return distribution(generator); });

  // Wait for all units
  array.barrier();

  perform_test(array.begin(), array.end());
}

TEST_F(SortTest, ArrayBlockedPartialRange)
{
  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  static std::uniform_int_distribution<Element_t> distribution(-1000, 1000);
  static std::random_device                       rd;
  static std::mt19937 generator(rd() + dash::myid());

  size_t num_local_elem = 100;

  LOG_MESSAGE("SortTest.ArrayBlockedPartialRange: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  std::generate(
      array.lbegin(), array.lend(), []() { return distribution(generator); });

  // Wait for all units
  array.barrier();

  auto begin = array.begin() + array.lsize() / 2;
  auto end   = array.end() - array.lsize() / 2;

  perform_test(begin, end);
}

TEST_F(SortTest, ArrayEmptyLocalRange)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units are required");
  }

  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  static std::uniform_int_distribution<Element_t> distribution(-100, 100);
  static std::random_device                       rd;
  static std::mt19937 generator(rd() + dash::myid());

  size_t num_local_elem = 10;

  LOG_MESSAGE("SortTest.ArrayBlockedPartialRange: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  std::generate(
      array.lbegin(), array.lend(), []() { return distribution(generator); });

  // Wait for all units
  array.barrier();

  perform_test(array.begin() + num_local_elem, array.end());
}

TEST_F(SortTest, ArrayOfDoubles)
{
  typedef double                 Element_t;
  typedef dash::Array<Element_t> Array_t;

  static std::uniform_real_distribution<Element_t> distribution(1.0, 2.0);
  static std::random_device                        rd;
  static std::mt19937 generator(rd() + dash::myid());

  size_t num_local_elem = 100;

  LOG_MESSAGE("SortTest.ArrayOfDoubles: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  std::generate(
      array.lbegin(), array.lend(), []() { return distribution(generator); });

  // Wait for all units
  array.barrier();

  perform_test(array.begin(), array.end());
}

template <typename GlobIter>
void perform_test(GlobIter begin, GlobIter end)
{
  using Element_t = typename decltype(begin)::value_type;

  auto const true_sum = dash::accumulate(begin, end, 0);

  dash::sort(begin, end);

  auto const actual_sum = dash::accumulate(begin, end, 0);

  if (dash::myid() == 0) {
    EXPECT_EQ_U(true_sum, actual_sum);

    for (auto it = begin + 1; it < end; ++it) {
      auto const a = static_cast<const Element_t>(*(it - 1));
      auto const b = static_cast<const Element_t>(*it);

      EXPECT_GE_U(b, a);
    }
  }
}

// TODO: add additional unit tests with various pattern types and containers
